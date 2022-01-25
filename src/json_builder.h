#pragma once

#include "json.h"

namespace json {

    class Builder;
    class DictItemContext;
    class KeyItemContext;
    class ArrayItemContext;

    class BaseContext {
    public:
        BaseContext(Builder& builder);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
        KeyItemContext Key(std::string key);
        Builder& Value(Node value);
    protected:
        Builder& builder_;
    };

    class DictItemContext : public BaseContext {
    public:
        DictItemContext(Builder& builder);
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
        Builder& EndArray() = delete;
        Builder& Value(Node value) = delete;
    };

    class KeyItemContext : public BaseContext {
    public:
        KeyItemContext(Builder& builder);
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        Builder& Key(std::string key) = delete;
        BaseContext Value(Node value);
    };

    class ArrayItemContext : public BaseContext {
    public:
        ArrayItemContext(Builder& builder);
        Builder& EndDict() = delete;
        KeyItemContext Key(std::string key) = delete;
        BaseContext Value(Node value);
    };  

    class Builder {
    public:
        Builder();

        DictItemContext StartDict();
        ArrayItemContext StartArray();

        Builder& EndDict();
        Builder& EndArray();

        KeyItemContext Key(std::string key);
        Builder& Value(Node value);

        Node Build();
    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
    };

}