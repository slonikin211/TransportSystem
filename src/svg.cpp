#include "svg.h"

namespace svg {
	using namespace std::literals;

	Point::Point(double x, double y) : x(x), y(y) {}

	RenderContext::RenderContext(std::ostream& out) : out(out) {}

	RenderContext::RenderContext(std::ostream& out, size_t indent_step, size_t indent) 
    : out(out), indent_step(indent_step), indent(indent) {}

	RenderContext RenderContext::Indented() const 
    {
		return {
			out, 
			indent_step, 
			indent + indent_step 
		};
	}

	void RenderContext::RenderIndent() const 
    {
		for (size_t i = 0u; i < indent; ++i)
        {
			out.put(' ');
		}
	}

	void Object::Render(const RenderContext& context) const 
    {
		context.RenderIndent();
		RenderObject(context);
		context.out << "\n"sv;
	}

	Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

	Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : Rgb(r, g, b), opacity(o) {}

	std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) 
    {
		switch (line_cap) 
        {
		case StrokeLineCap::BUTT:
			out << "butt"sv;
			break;
		case StrokeLineCap::ROUND:
			out << "round"sv;
			break;
		case StrokeLineCap::SQUARE:
			out << "square"sv;
			break;
		default:
			break;
		}

		return out;
	}

	std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) 
    {
		switch (line_join) 
        {
		case StrokeLineJoin::ARCS:
			out << "arcs"sv;
			break;
		case StrokeLineJoin::BEVEL:
			out << "bevel"sv;
			break;
		case StrokeLineJoin::MITER:
			out << "miter"sv;
			break;
		case StrokeLineJoin::MITER_CLIP:
			out << "miter-clip"sv;
			break;
		case StrokeLineJoin::ROUND:
			out << "round"sv;
			break;
		default:
			break;
		}
		return out;
	}

	Circle& Circle::SetCenter(Point center) 
    {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius) 
    {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const 
    {
		auto& out = context.out;
		out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
		out << "r=\""sv << radius_ << "\""sv;
		RenderAttrs(out);
		out << "/>"sv;
	}

	Polyline& Polyline::AddPoint(Point point) 
    {
		points_.emplace_back(std::move(point));
		return *this;
	}

	void Polyline::RenderObject(const RenderContext& context) const 
    {
		auto& out = context.out;
		out << "<polyline points=\""sv;
		const size_t points_size = points_.size();
		for (size_t i = 0u; i < points_size; ++i) 
        {
			const auto& point = points_[i];
			if (i) 
            {
				out << " "sv;
			}
			out << point.x << ","sv << point.y;
		}
		out << "\""sv;
		RenderAttrs(out);
		out << "/>"sv;
	}

	Text& Text::SetPosition(Point pos) 
    {
		position_ = std::move(pos);
		return *this;
	}

	Text& Text::SetOffset(Point offset) 
    {
		offset_ = std::move(offset);
		return *this;
	}

	Text& Text::SetFontSize(uint32_t size) 
    {
		font_size_ = std::move(size);
		return *this;
	}

	Text& Text::SetFontFamily(std::string font_family) 
    {
		font_family_ = std::move(font_family);
		return *this;
	}

	Text& Text::SetFontWeight(std::string font_weight) 
    {
		font_weight_ = std::move(font_weight);
		return *this;
	}

	Text& Text::SetData(std::string data) 
    {
		data_ = std::move(data);
		return *this;
	}

	void Text::RenderObject(const RenderContext& context) const 
    {
		auto& out = context.out;
		out << "<text"sv;
		RenderAttrs(out);
		out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
		out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
		out << " font-size=\""sv << font_size_ << "\""sv;
		if (font_family_) { out << " font-family=\""sv << *font_family_ << "\""sv; }
		if (font_weight_) { out << " font-weight=\""sv << *font_weight_ << "\""sv; }
		out << ">"sv;
		
        std::string text;
		for (const char letter : data_) 
        {
			switch (letter) 
            {
			case '\"':
				text += "&quot;"s;
				break;
			case '\'':
				text += "&apos;"s;
				break;
			case '<':
				text += "&lt;"s;
				break;
			case '>':
				text += "&gt;"s;
				break;
			case '&':
				text += "&amp"s;
				break;
			default:
				text += letter;
			}
		}
		out << std::move(text) << "</text>"sv;
	}

	void Document::AddPtr(std::unique_ptr<Object>&& obj) 
    {
		objects_.emplace_back(std::move(obj));
	}

	void Document::Render(std::ostream& out) const 
    {
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
		for (const auto& object : objects_) 
        {
			object.get()->Render(RenderContext(out, 2, 2));
		}
		out << "</svg>"sv;
	}

}
