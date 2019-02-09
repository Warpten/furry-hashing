#pragma once

#include "string_view_range.hpp"
#include "uploaded_string.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <string_view>
#include <cctype>
#include <functional>

struct node_t {
    virtual ~node_t() { }

    virtual uint32_t apply(std::vector<char>& storage, uint32_t offset) = 0;
    virtual std::string_view parse(std::string_view view) = 0;

    virtual void reset() = 0;
    virtual bool has_next() = 0;
    virtual void move_next() = 0;

    virtual uint32_t count() = 0;

    virtual std::vector<std::string> const& values() = 0;

    node_t* next = nullptr;
};

// raw characters
struct raw_range_t final : public node_t {
    virtual ~raw_range_t() { }

private:
    std::vector<std::string> val;

public:
    uint32_t apply(std::vector<char>& storage, uint32_t offset) override;
    std::string_view parse(std::string_view view) override;

    void reset() override { }
    bool has_next() override { return false; }
    void move_next() override { }

    uint32_t count() override { return 1; }

    virtual std::vector<std::string> const& values() {
        return val;
    }
};

// size modifier {x, y} {x}
struct size_specified_range_t : public node_t {
    virtual ~size_specified_range_t() { }

protected:
    uint32_t min_count;
    uint32_t max_count;

public:
    virtual uint32_t apply(std::vector<char>& storage, uint32_t offset) = 0;
    std::string_view parse(std::string_view view) override;

    virtual void reset() = 0;
    virtual bool has_next() = 0;
    virtual void move_next() = 0;

    virtual uint32_t count() = 0;
};

// array (x|y|z)
struct array_range_t final : public size_specified_range_t {
    virtual ~array_range_t() { }

private:
    std::vector<std::string> vals;
    decltype(vals)::const_iterator itr;

public:
    uint32_t apply(std::vector<char>& storage, uint32_t offset) override;
    std::string_view parse(std::string_view view) override;

    void reset() override;
    bool has_next() override;
    void move_next() override;

    uint32_t count() { return vals.size(); }

    std::vector<std::string> const& values() override { return vals; }
};

// ranges [a-z|alpha|alnum|num|hex|path]
struct varying_range_t : public size_specified_range_t {
    virtual ~varying_range_t() { }

private:
    std::set<char> universe;

    std::vector<std::string> vals;
    decltype(vals)::const_iterator itr;

public:
    uint32_t apply(std::vector<char>& storage, uint32_t offset) override;
    std::string_view parse(std::string_view view) override;
    void reset() override;
    bool has_next() override;
    void move_next() override;

    uint32_t count() { return vals.size(); }

    std::vector<std::string> const& values() override { return vals; }

private:
    void generate_perms();
    std::vector<std::string> generate_perms(uint32_t minCount, uint32_t maxCount);
};

struct pattern_t {
private:
    node_t* head = nullptr;
    node_t* tail = nullptr;

    std::vector<string_view_range<char>> vals;
    decltype(vals)::const_iterator itr;

public:
    pattern_t(std::string_view regex);

    ~pattern_t() {
		//TODO: blowing up the stack here would be fun; wouldn't it?
        std::function<void(node_t*)> last_deleter;
        last_deleter = [&](node_t* node) -> void {
            if (node->next != nullptr) {
                last_deleter(node->next);
                node->next = nullptr;
            }
            delete node;
        };

        last_deleter(head);
        tail = nullptr;
    }

    uint32_t count();

    void collect(std::vector<uploaded_string>& bucket);
};

