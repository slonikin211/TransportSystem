#include "svg.h"

namespace svg {
using namespace std::literals;

// Out
std::ostream& operator<<(std::ostream& os, const std::optional<StrokeLineCap>& line_cap)
{
    using namespace std::string_view_literals;
    switch (line_cap.value())
    {
    case StrokeLineCap::BUTT: os << "butt"sv; break;
    case StrokeLineCap::ROUND: os << "round"sv; break;
    case StrokeLineCap::SQUARE: os << "square"sv; break;
    }
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::optional<StrokeLineJoin>& line_join)
{
    using namespace std::string_view_literals;
    switch (line_join.value())
    {
    case StrokeLineJoin::ARCS: os << "arcs"sv; break;
    case StrokeLineJoin::BEVEL: os << "bevel"sv; break;
    case StrokeLineJoin::MITER: os << "miter"sv; break;
    case StrokeLineJoin::MITER_CLIP: os << "miter-clip"sv; break;
    case StrokeLineJoin::ROUND: os << "round"sv; break;
    }
    return os;
}

void ColorPrinter::operator()(std::monostate) const {
    out << "none"sv;
}
void ColorPrinter::operator()(std::string color) const {
    out << color;
}
void ColorPrinter::operator()(Rgb color) const {
    out << "rgb("sv << static_cast<size_t>(color.red) << ","sv << static_cast<size_t>(color.green) << ","sv << static_cast<size_t>(color.blue) << ")"sv; 
}
void ColorPrinter::operator()(Rgba color) const {
    out << "rgba("sv << static_cast<size_t>(color.red) << ","sv << static_cast<size_t>(color.green) << ","sv << static_cast<size_t>(color.blue) << ","sv << color.opacity << ")"sv;
}


void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << '\n';
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    // PathProps attrs
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point)
{
    line_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const
{
    auto& out = context.out;
    out << "<polyline points=\""sv;

    for (auto it = line_.begin(); it != line_.end(); ++it)
    {
        if (it == line_.begin())
        {
            out << it->x << ","sv << it->y;
            continue;
        }
        out << " "sv << it->x << ","sv << it->y;
    }
    out << "\" "sv;
    // PathProps attrs
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos)
{
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset)
{
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size)
{
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family)
{
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight)
{
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data)
{
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const
{
    context.RenderIndent();
    auto& out = context.out;
    out << "<text "sv;

    // PathProps attrs
    RenderAttrs(context.out);

    out << "x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << size_ << "\""sv; //1` as a default

    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    //out.seekp(-1, std::ios_base::end);  // remove last space
    out << ">"sv;
    
    for (const char c : data_)
    {
        if (c == '\"') { out << "&quot;"sv; }
        else if (c == '\'') { out << "&apos;"sv; }
        else if (c == '<') { out << "&lt;"sv; }
        else if (c == '>') { out << "&gt;"sv; }
        else if (c == '&') { out << "&amp;"sv; }
        else { out << c; }
    }

    out << "</text>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj)
{
    objs_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const
{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"s << '\n';
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"s << '\n';

    RenderContext context(out, 2, 2);
    for (const auto& obj: objs_)
    {
        obj->Render(context);
    }

    out << "</svg>"s;
}
}  // namespace svg