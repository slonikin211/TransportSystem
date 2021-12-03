#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cstddef>
#include <variant>

namespace json
{
    // Node content using std::variant
    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using NodeContent = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    struct NodeInterpretor
    {
        const Array& operator()(const Array& array) const;
        const Dict& operator()(const Dict& dict) const;
        bool operator()(bool cond) const;
        int operator()(int num) const;
        double operator()(double num) const;
        const std::string& operator()(const std::string& str) const;
    };

    struct NodeOutput
    {
        void operator()(std::nullptr_t n) const;
        void operator()(int n) const;
        void operator()(double n) const;
        void operator()(bool b) const;
        void operator()(const std::string& str) const;
        void operator()(const Array& arr) const;
        void operator()(const Dict& dict) const;

        std::ostream& out;
    };

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        Node() = default;

        // Constructors
        Node(Array array) : content_(std::move(array)) {}
        Node(Dict map) : content_(std::move(map)) {}
        Node(int value) : content_(value) {}
        Node(bool value) : content_(value) {}
        Node(double dbl) : content_(dbl) {}
        Node(std::nullptr_t n) : content_(n) {}
        Node(std::string str) : content_(std::move(str)) {}

        // AsType
        int AsInt() const;
        double AsDouble() const;
        const std::string& AsString() const;
        bool AsBool() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        // IsType
        bool IsNull() const;
        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsString() const;
        bool IsBool() const;
        bool IsArray() const;
        bool IsMap() const;

        // Other
        const NodeContent& GetRootContent() const;

        // Operators
        bool operator==(const Node& other) const;
        bool operator!=(const Node& other) const;

    private:
        NodeContent content_;
    };



    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

        bool operator==(const Document& other) const;
        bool operator!=(const Document& other) const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

}  // namespace json
