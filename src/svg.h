#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <string_view>
#include <utility>

namespace svg 
{
	struct Point 
    {
		Point() = default;
		Point(double x, double y);

		double x = 0.0;
		double y = 0.0;
	};

	struct RenderContext 
    {
		RenderContext(std::ostream& out);
		RenderContext(std::ostream& out, size_t indent_step, size_t indent = 0u);

		RenderContext Indented() const;
		void RenderIndent() const;

		std::ostream& out;
		size_t indent_step = 0u;
		size_t indent = 0u;
	};

	class Object 
    {
	public:
		void Render(const RenderContext& context) const;
		virtual ~Object() = default;

	private:
		virtual void RenderObject(const RenderContext& context) const = 0;
	};

	enum class StrokeLineCap 
    {
		BUTT,
		ROUND,
		SQUARE,
	};

	enum class StrokeLineJoin 
    {
		ARCS,
		BEVEL,
		MITER,
		MITER_CLIP,
		ROUND,
	};

	struct Rgb 
    {
		Rgb() = default;
		Rgb(uint8_t r, uint8_t g, uint8_t b);

		uint8_t red = 0;
		uint8_t	green = 0;
		uint8_t	blue  = 0;
	};

	struct Rgba : public Rgb 
    {
		Rgba() = default;
		Rgba(uint8_t r, uint8_t g, uint8_t b, double o);

		double opacity = 1.0;
	};

	using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

	inline const Color NoneColor{};

	std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);
	std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

	template <typename Owner>
	class PathProps 
    {
	public:
		Owner& SetFillColor(Color color) 
        {
			fill_color_ = std::move(color);
			return AsOwner();
		}
		Owner& SetStrokeColor(Color color)
        {
			stroke_color_ = std::move(color);
			return AsOwner();
		}
		Owner& SetStrokeWidth(double width) 
        {
			stroke_width_ = width;
			return AsOwner();
		}
		Owner& SetStrokeLineCap(StrokeLineCap line_cap)
        {
			line_cap_ = line_cap;
			return AsOwner();
		}
		Owner& SetStrokeLineJoin(StrokeLineJoin line_join)
        {
			line_join_ = line_join;
			return AsOwner();
		}

	protected:
		~PathProps() = default;

		void RenderAttrs(std::ostream& out) const 
        {
			using namespace std::literals;

			if (fill_color_) 
            {
				out << " fill=\""sv;
				std::visit(ColorPrinter{ out }, *fill_color_);
				out << "\""sv;
			}
			if (stroke_color_) 
            {
				out << " stroke=\""sv;
				std::visit(ColorPrinter{ out }, *stroke_color_);
				out << "\""sv;
			}
			if (stroke_width_) 
            {
				out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
			}
			if (line_cap_) 
            {
				out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
			}
			if (line_join_) 
            {
				out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
			}
		}

	private:
		std::optional<Color> fill_color_;
		std::optional<Color> stroke_color_;
		std::optional<double> stroke_width_;
		std::optional<StrokeLineCap> line_cap_;
		std::optional<StrokeLineJoin> line_join_;

		Owner& AsOwner()
        {
			return static_cast<Owner&>(*this);
		}

		struct ColorPrinter 
        {
			std::ostream& out;

			void operator()(std::monostate) const 
            {
				using namespace std::literals;
				out << "none"sv;
			}
			void operator()(const std::string_view str) const 
            {
				using namespace std::literals;
				out << str;
			}
			void operator()(const Rgb& rgb) 
            {
				using namespace std::literals;
				out << "rgb("sv
					<< (int)rgb.red   << ","sv
					<< (int)rgb.green << ","sv
					<< (int)rgb.blue  << ")"sv;
			}
			void operator()(const Rgba& rgba) 
            {
				using namespace std::literals;
				out << "rgba("sv
					<< (int)rgba.red    << ","sv
					<< (int)rgba.green  << ","sv
					<< (int)rgba.blue   << ","sv
					<< rgba.opacity     << ")"sv;
			}
		};
	};

	class Circle final : public Object, public PathProps<Circle> 
    {
	public:
		Circle& SetCenter(Point center);
		Circle& SetRadius(double radius);

	private:
		Point center_;
		double radius_ = 1.0;

		void RenderObject(const RenderContext& context) const override;
	};

	class Polyline : public Object, public PathProps<Polyline> 
    {
	public:
		Polyline& AddPoint(Point point);

	private:
		std::vector<Point> points_;

		void RenderObject(const RenderContext& context) const override;
	};

	class Text : public Object, public PathProps<Text> 
    {
	public:
		Text& SetPosition(Point pos);
		Text& SetOffset(Point offset);
		Text& SetFontSize(uint32_t size);
		Text& SetFontFamily(std::string font_family);
		Text& SetFontWeight(std::string font_weight);
		Text& SetData(std::string data);

	private:
		Point position_;
		Point offset_;
		uint32_t font_size_ = 1;
		std::optional<std::string> font_family_;
		std::optional<std::string> font_weight_;
		std::string data_;

		void RenderObject(const RenderContext& context) const override;
	};

	class ObjectContainer 
    {
	public:
		template <typename Obj>
		void Add(Obj&& object) 
        {
			AddPtr(std::move(std::make_unique<Obj>(std::move(object))));
		}

		virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
		virtual ~ObjectContainer() = default;
	};

	class Drawable 
    {
	public:
		virtual void Draw(ObjectContainer& container) const = 0;
		virtual ~Drawable() = default;
	};

	class Document : public ObjectContainer 
    {
	public:
		void AddPtr(std::unique_ptr<Object>&& obj);
		void Render(std::ostream& out) const;

	private:
		std::vector<std::unique_ptr<Object>> objects_;
	};

}