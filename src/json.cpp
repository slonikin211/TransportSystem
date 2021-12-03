#include "json.h"

#include <math.h>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

const double EPS = 0.000001;
inline bool dequal(double a, double b) {
    return std::fabs(a - b) < EPS;
}

string trim(const string& str) {
    string_view sv = str;
    if (sv.empty()) { return str; }
    while (::isspace(sv.front())) { sv.remove_prefix(1u); }
    while (::isspace(sv.back())) { sv.remove_suffix(1u); }
    return string(sv);
}

namespace json {

    // Interpreter block
    const Array& NodeInterpretor::operator()(const Array& array) const {
        return array;
    }
    const Dict& NodeInterpretor::operator()(const Dict& dict) const {
        return dict;
    }
    bool NodeInterpretor::operator()(bool cond) const {
        return cond;
    }
    int NodeInterpretor::operator()(int num) const {
        return num;
    }
    double NodeInterpretor::operator()(double num) const {
        return num;
    }
    const std::string& NodeInterpretor::operator()(const std::string& str) const {
        return str;
    }

    // Node out block
    void NodeOutput::operator()(std::nullptr_t n) const {
        (void)&n;
        out << "null";
    }
    void NodeOutput::operator()(int n) const {
        out << n;
    }
    void NodeOutput::operator()(double n) const {
        out << n;
    }
    void NodeOutput::operator()(bool b) const {
        out << boolalpha << b << noboolalpha;
    }
    void NodeOutput::operator()(const std::string& str) const {
        out << "\"";
        for (const auto c : str) {
            if (c == '\n') { out << "\\\n"; }
            else if (c == '\r') { out << "\\\r"; }
            else if (c == '\"') { out << "\\\""; }
            else if (c == '\t') { out << "\\\t"; }
            else if (c == '\\') { out << "\\\\"; }
            else { out << c; }
        }
        out << "\"";
    }
    void NodeOutput::operator()(const Array& arr) const {
        out << "[";

        for (auto it = arr.begin(); it != arr.end(); ++it) {
            if (it == arr.begin()) {
                std::visit(NodeOutput{ out }, it->GetRootContent());
                continue;
            }
            out << ',';
            std::visit(NodeOutput{ out }, it->GetRootContent());
        }

        out << "]";
    }
    void NodeOutput::operator()(const Dict& dict) const {
        out << "{ ";

        for (auto it = dict.begin(); it != dict.end(); ++it) {
            if (it == dict.begin()) {
                out << "\""sv << it->first << "\": "sv;
                std::visit(NodeOutput{ out }, it->second.GetRootContent());
                continue;
            }
            out << ',' << ' ';
            out << "\""sv << it->first << "\": "sv;
            std::visit(NodeOutput{ out }, it->second.GetRootContent());
        }

        out << " }";
    }

    namespace {
        /*
            Load block
        */
        using Number = std::variant<int, double>;
        Number LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            auto skip_spaces = [&input](){
                while (::isspace(static_cast<char>(input.peek()))) { input.get(); }
            };


            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else {
                read_digits();
            }
            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                skip_spaces();
                if (!input.eof()) {
                    read_digits();
                }
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadNode(istream& input);

        Node LoadNull(istream& input)
        {
            using namespace std::literals;
            std::string parsed_null;

            // Read chars until the end of input
            while (!input.eof()) {
                parsed_null += input.get();
            }
            parsed_null.resize(parsed_null.size() - 1u);    // remove EOF symbol '\377'

            if (parsed_null == "ull"s)
            {
                return Node(nullptr);
            }
            throw ParsingError("This is not a null");
        }

        // Only for LoadString function for replacement escape sequences
        void replace_substr_from_str(std::string& str, std::string_view to_be_replace, std::string_view replace_by) {
            size_t index = 0;
            while (true) {
                // Locate the substring to replace
                index = str.find(to_be_replace, index);
                if (index == std::string::npos) break;

                // Make the replacement
                str.replace(index, 2u, replace_by);

                // Advance index forward so the next iteration doesn't pick it up as well
                index += 2u;
            }
        }

        void replace_some_esc_seq(std::string& str) {
            // Zip escape sequences (\\\" -> \")
            replace_substr_from_str(str, "\\\""sv, "\""sv);
            replace_substr_from_str(str, "\\n"sv, "\n"sv);
            replace_substr_from_str(str, "\\r"sv, "\r"sv);
            replace_substr_from_str(str, "\\t"sv, "\t"sv);
            replace_substr_from_str(str, "\\\\"sv, "\\"sv);
        }

        Node LoadString(istream& input) {
            std::string parsed_string(std::istreambuf_iterator<char>(input), {});
            
            // Check correct input format
            if (parsed_string.find('\"') == parsed_string.npos) { throw ParsingError("Incorrect string format!"s); }

            parsed_string.pop_back(); // remove last '\"'

            // Zip escape sequences (\\\" -> \")
            replace_some_esc_seq(parsed_string);
            return Node(move(parsed_string));
        }

        Node LoadBool(istream& input) {
            string node_name(std::istreambuf_iterator<char>(input), {});
            node_name = trim(node_name);
            if (node_name == /* true */ "rue"s) { return Node(true); }
            if (node_name == /* false */ "alse"s) { return Node(false); }
            throw ParsingError("Incorrect bool format");
        }

        Node LoadArray(istream& input) {
            Array result;

            // First of all need to parse value: between (',' or ']') and use trim
            // Secondly, need to determine a type of a value (or just use LoadNode instantly)
            // Thirdly save a value with correct type to the result
            // Additional: if ']' is not found => incorrect input format for array
            
            // Help function for skip space-like chars from the input
            auto skip_spaces = [&input](){
                while (::isspace(static_cast<char>(input.peek()))) { input.get(); }
            };

            // Help function for getting value from input for array
            auto get_value = [&input, skip_spaces](){
                skip_spaces();
                
                const char peek = static_cast<char>(input.peek());
                bool special_type = (peek == '\"' || peek == '{' || peek == '[');
                
                string value;
                if (special_type) {
                    if (peek == '\"') {
                        // Save first char '\"'
                        char c = static_cast<char>(input.get());
                        value += c;
                        // Read until '\"' (without prev char '\\')
                        while (true) {
                            char c = static_cast<char>(input.get());
                            if (c == '\"' && value.back() != '\\') {    // string end
                                value += c;
                                return value;
                            }
                            if (input.eof()) { throw ParsingError("Incorrect string format in the array"); }
                            value += c;
                        }
                    }
                    else {
                        // Using delemiter count (if zero then value is ready)
                        char c = static_cast<char>(input.get());
                        const char del_start = c;
                        const char del_end = (del_start == '[') ? (']') : ('}');
                        value += c;
                        size_t del_counter = 1u;

                        while (true) {
                            c = static_cast<char>(input.get());
                            if (c == del_start)     { del_counter++; }
                            else if (c == del_end)  { del_counter--; }
                            
                            // save symbol into a value
                            value += c;

                            // value is ready
                            if (del_counter == 0) {
                                return value;
                            }

                            // Invalid array/dict (del_counter is not 0 before input end)
                            if (input.eof()) { throw ParsingError("Invalid container type (array/dictionary) in the array"); }
                        }
                    }
                }
                else {
                    while (static_cast<char>(input.peek()) != ',' && static_cast<char>(input.peek()) != ']') {
                        char c = input.get();
                        if (input.eof()) { throw ParsingError("Invalid array format"); }
                        value += c;
                    }
                }
                skip_spaces();

                value = trim(value);
                return value;
            };


            // Start parse
            // 1. Read istream until ']'
            while (static_cast<char>(input.peek()) != ']') {
            // If ']' is not found then parse error
            if (input.eof()) { throw ParsingError("Invalid array format"); }

                // 2.Get value (until ',' or ']')
                string value = get_value();

                // 3. Save data with the correct type
                stringstream ss(value);
                result.push_back(LoadNode(ss));

                // Next iteration or break
                skip_spaces();
                if (static_cast<char>(input.peek()) == ']') { break; }
                input.get();    // remove ','
            }

            return Node(move(result));
        }

        Node LoadDict(istream& input) {
            Dict result;

            // First of all need to parse pair: between (',' or '}') and use trim
            // Secondly, need to determine a type of a value (or just use LoadNode instantly) after parsing a key
            // Thirdly save a pair with correct value type to the result
            // Additional: if '}' is not found => incorrect input format for map
            
            // Help function for skip space-like chars from the input
            auto skip_spaces = [&input](){
                while (::isspace(static_cast<char>(input.peek()))) { input.get(); }
            };

            // Help function for delete chars at the begining and at the end (for "key")
            auto remove_border_chars = [](string& str) {
                string_view sv = str;
                sv.remove_prefix(1u);
                sv.remove_suffix(1u);
                str = string(sv);
            };

            auto get_pair = [&input, skip_spaces, remove_border_chars](){
                skip_spaces();

                // Key is always string
                if (static_cast<char>(input.peek()) != '\"') { throw ParsingError("Incorrect key in the map"); }

                // 1. Getting key (until ':' and then trim)
                string key;
                while (true) {
                    char c = static_cast<char>(input.get());
                    if (c == ':') { // key is ready here
                        key = trim(key);
                        break;
                    }
                    if (input.eof()) { throw ParsingError("Pair in the map has no \':\'"); }
                    key += c;
                }
                // 1.1. Remove \" at the begining and at the end
                remove_border_chars(key);
                


                // 2. Get value (simillar as in array)
                skip_spaces();

                const char peek = static_cast<char>(input.peek());
                bool special_type = (peek == '\"' || peek == '{' || peek == '[');
                
                string value;
                if (special_type) {
                    if (peek == '\"') {
                        // Save first char '\"'
                        char c = static_cast<char>(input.get());
                        value += c;
                        // Read until '\"' (without prev char '\\')
                        while (true) {
                            char c = static_cast<char>(input.get());
                            if (c == '\"' && value.back() != '\\') {    // string end
                                value += c;
                                break;
                            }
                            if (input.eof()) { throw ParsingError("Incorrect string format in the array"); }
                            value += c;
                        }
                    }
                    else {
                        // Using delemiter count (if zero then value is ready)
                        char c = static_cast<char>(input.get());
                        const char del_start = c;
                        const char del_end = (del_start == '[') ? (']') : ('}');
                        value += c;
                        size_t del_counter = 1u;

                        while (true) {
                            c = static_cast<char>(input.get());
                            if (c == del_start)     { del_counter++; }
                            else if (c == del_end)  { del_counter--; }
                            
                            // save symbol into a value
                            value += c;

                            // value is ready
                            if (del_counter == 0) {
                                break;
                            }

                            // Invalid array/dict (del_counter is not 0 before input end)
                            if (input.eof()) { throw ParsingError("Invalid container type (array/dictionary) in the array"); }
                        }
                    }
                }
                else {
                    while (static_cast<char>(input.peek()) != ',' && static_cast<char>(input.peek()) != '}') {
                        char c = static_cast<char>(input.get());
                        if (input.eof()) { throw ParsingError("Invalid array format"); }
                        value += c;
                    }
                }
                
                skip_spaces();
                return trim(key) + ":"s + trim(value);
            };

            // Start parse
            // 1. Read istream until '}'
            while (static_cast<char>(input.peek()) != '}') {
            // If '}' is not found then parse error
            if (input.eof()) { throw ParsingError("Invalid dict format"); }

                // 2.Get pair (until ',' or '}')
                string pair = get_pair();   // "key":value

                // Get key and value
                size_t del_pos = pair.find(':');
                string key = pair.substr(0u, del_pos);
                string value = pair.substr(del_pos + 1u, pair.size() - key.size() + 1u);

                // 3. Save data with the correct type
                stringstream ss(value);
                result.insert({key, LoadNode(ss)});

                // Next iteration or break
                skip_spaces();
                if (static_cast<char>(input.peek()) == '}') { break; }
                input.get();    // remove ','
            }

            
            return Node(move(result));
        }

        Node LoadNode(istream& input) {
            // Help function for skip space-like chars from the input
            auto skip_spaces = [&input](){
                while (::isspace(static_cast<char>(input.peek()))) { input.get(); }
            };

            skip_spaces();
            char c;
            input >> c;

            if (c == 'n') {     // null 
                return LoadNull(input);
            }
            else if (c == 't' || c == 'f') {  // true
                return LoadBool(input);
            }
            else if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                throw ParsingError("Incorrect input data");
            }
            
            else {
                input.putback(c);
                Number num = LoadNumber(input);
                if (std::holds_alternative<int>(num)) {
                    return Node(std::get<int>(num));
                }
                else {
                    return Node(std::get<double>(num));
                }
            }
        }

    }  // namespace


    // As type block

    int Node::AsInt() const {
        if (!IsInt()) {
            throw logic_error("This is not int"); 
        }
        return std::get<int>(content_);
    }

    double Node::AsDouble() const {
        if (std::holds_alternative<int>(content_)) {
            return static_cast<double>(std::get<int>(content_));
        }
        if (!IsDouble()) { throw logic_error("This is not double"); }
        return std::get<double>(content_);
    }

    const std::string& Node::AsString() const {
        if (!IsString()) { throw logic_error("This is not string"); }
        return std::get<std::string>(content_);
    }

    bool Node::AsBool() const {
        if (!IsBool()) { throw logic_error("This is not bool"); }
        return std::get<bool>(content_);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) { throw logic_error("This is not array"); }
        return std::get<Array>(content_);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) { throw logic_error("This is not dictionary"); }
        return std::get<Dict>(content_);
    }


    // Is type block

    bool Node::IsNull() const {
        return std::holds_alternative<nullptr_t>(content_);
    }

    bool Node::IsInt() const {
        return std::holds_alternative<int>(content_);
    }

    bool Node::IsDouble() const {
        return std::holds_alternative<double>(content_) || std::holds_alternative<int>(content_);
    }

    bool Node::IsPureDouble() const {
        if (IsInt()) {
            return false;
        }
        if (IsDouble()) {
            return true;
        }
        // else if (IsDouble()) {
        //     auto val = std::get<double>(content_);
        //     auto recasted = static_cast<double>(static_cast<int>(val));
        //     return !dequal(val, recasted);
        // }
        // // else {
        // //     throw std::runtime_error("Not a number type");
        // // }
        return false;
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(content_);
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(content_);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(content_);
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(content_);
    }


    // Operators and other

    bool Node::operator==(const Node& other) const {
        // Nullptr
        if (std::holds_alternative<nullptr_t>(content_) && std::holds_alternative<nullptr_t>(other.content_)) {
            return true;
        }

        // Int
        if (std::holds_alternative<int>(content_) && std::holds_alternative<int>(other.content_)) {
            return std::get<int>(content_) == std::get<int>(other.content_);
        }

        // Double
        if (std::holds_alternative<double>(content_) && std::holds_alternative<double>(other.content_)) {
            return dequal(std::get<double>(content_), std::get<double>(other.content_));
        }

        // String
        if (std::holds_alternative<std::string>(content_) && std::holds_alternative<std::string>(other.content_)) {
            return std::get<std::string>(content_) == std::get<std::string>(other.content_);
        }

        // Bool
        if (std::holds_alternative<bool>(content_) && std::holds_alternative<bool>(other.content_)) {
            return std::get<bool>(content_) == std::get<bool>(other.content_);
        }

        // Array
        if (std::holds_alternative<Array>(content_) && std::holds_alternative<Array>(other.content_)) {
            return std::get<Array>(content_) == std::get<Array>(other.content_);
        }

        // Dictionary
        if (std::holds_alternative<Dict>(content_) && std::holds_alternative<Dict>(other.content_)) {
            return std::get<Dict>(content_) == std::get<Dict>(other.content_);
        }
        return false;
    }

    bool Node::operator!=(const Node& other) const {
        return !(*this == other);
    }

    const NodeContent& Node::GetRootContent() const {
        return content_;
    }


    // Document

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    bool Document::operator==(const Document& other) const {
        return (GetRoot() == other.GetRoot());
    }

    bool Document::operator!=(const Document& other) const {
        return !(*this == other);
    }

    void Print(const Document& doc, std::ostream& output) {
        const Node& root = doc.GetRoot();

        std::visit(NodeOutput{ output }, root.GetRootContent());
        (void)&doc;
        (void)&output;

        // Реализуйте функцию самостоятельно
    }

}  // namespace json