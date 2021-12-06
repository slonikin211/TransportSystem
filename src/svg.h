#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <deque>
#include <optional>
#include <variant>

namespace svg 
{

/*
    Color module
*/

struct Rgb
{
    Rgb() : red(0u), green(0u), blue(0u) {}
    Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
    uint8_t red, green, blue;
};

struct Rgba : Rgb   // public inheritance as default for structs
{
    Rgba() : Rgb(), opacity(1.0) {}
    Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : Rgb(r, g, b), opacity(o) {}
    double opacity;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
static const Color NoneColor;

struct ColorPrinter
{
    std::ostream& out;
    void operator()(std::monostate) const;
    void operator()(std::string color) const;
    void operator()(Rgb color) const;
    void operator()(Rgba color) const;
};


/*
    Stoke module
*/

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};
enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& os, const std::optional<StrokeLineCap>& line_cap);
std::ostream& operator<<(std::ostream& os, const std::optional<StrokeLineJoin>& line_join);

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        // for (int i = 0; i < indent; ++i) {
        //     out.put(' ');
        // }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};



// Interfaces

class ObjectContainer
{
public:
    // void Add(???);
    template <typename Obj>
    void Add(Obj obj)
    {
        objs_.push_back(std::make_unique<Obj>(std::move(obj)));
    }
    
    // Добавляет в svg-документ объект-наследник svg::Object
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
    ~ObjectContainer() = default;

    std::deque<std::unique_ptr<Object>> objs_;
};
class Drawable
{
public:
    virtual ~Drawable() = default;

    virtual void Draw(ObjectContainer& object_container) const = 0;
};

template <class Owner>
class PathProps
{
public:
    Owner& SetFillColor(Color fill_color)
    {
        fill_color_ = std::move(fill_color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color stroke_color)
    {
        stroke_color_ = std::move(stroke_color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width)
    {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap stroke_line_cap)
    {
        line_cap_ = stroke_line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin stroke_line_join)
    {
        line_join_ = stroke_line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;
    
    void RenderAttrs(std::ostream& out) const 
    {
        using namespace std::literals;
        if (fill_color_) {
            out << " fill=\""sv;
            std::visit(ColorPrinter{out}, *fill_color_); 
            out << "\" "sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv;
            std::visit(ColorPrinter{out}, *stroke_color_); 
            out << "\" "sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << stroke_width_.value() << "\" "sv;
        }
        if (line_cap_) {
            out << " stroke-linecap=\""sv << line_cap_ << "\" "sv;
        }
        if (line_join_) {
            out << " stroke-linejoin=\""sv << line_join_ << "\" "sv;
        }
    }

private:
    Owner& AsOwner()
    {
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_, stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle>
{
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline>
{
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::deque<Point> line_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text>
{
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    Point position_, offset_;
    uint32_t size_ = 1u;
    std::string font_family_, font_weight_, data_;

    void RenderObject(const RenderContext& context) const override;
};

class Document : public ObjectContainer
{
public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;
};

}  // namespace svg