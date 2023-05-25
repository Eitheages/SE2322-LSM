#ifndef SKIPLIST_CLASS
#define SKIPLIST_CLASS

#include <cassert>
#include <type_traits>
#include <vector>
#include <limits>
#include <ctime>
#include <cstdlib>
#include <cstdint>

namespace basic_ds {
using size_type = std::size_t;

// Only for numerical key_type
template <typename _key_type, typename _value_type,
          typename = std::enable_if_t<std::is_integral<_key_type>::value>>
class SkipList {
    // The inner data struct node.
    using key_type = _key_type;
    using value_type = _value_type;
    struct SkipListNode {
        friend class SkipList;

    public:
        key_type key;
        value_type val;

        SkipListNode(const key_type &k, const value_type &v, SkipListNode *p1 = nullptr,
                     SkipListNode *p2 = nullptr, SkipListNode *p3 = nullptr,
                     SkipListNode *p4 = nullptr)
            : key(k), val(v), _pre(p1), _next(p2), _above(p3), _below(p4) {}

    private:
        SkipListNode *_pre, *_next, *_above, *_below;
    };

    using Node = struct SkipListNode;

public:
    using kv_type = std::pair<key_type, value_type>;

    explicit SkipList() : h{0} {
        std::srand(time(nullptr));
        head.push_back(new Node(MIN_KEY, value_type{}));
        tail.push_back(new Node(MAX_KEY, value_type{}));
        head[0]->_next = tail[0];
        tail[0]->_pre = head[0];
    }
    ~SkipList() {
        for (int i = 0; i <= h; ++i) {
            Node *p = head[i];
            do {
                p = p->_next;
                delete p->_pre;
            } while (p->_next);
            delete p;
        }
    }

    // Returns: a pair consisting of: the node inserted, or the node prevent the insertion,
    //          and a bool flag indicates whether the insertion is successfully performed.
    std::pair<const Node *, bool> insert(const key_type &key, const value_type &val) {
        Node *t;
        if (this->searchUtil(key, &t)) {
            assert(t->key == key);
            return {t, false};
        }
        assert(t->key < key || (key == MIN_KEY && t == head[0]));
        this->insertUntil(key, val, t);
        return {t->_next, true};
    }

    // Returns: a pair consisting of: the node inserted or updated,
    //          and a flag, true if the insertion took place and false if the update took place.
    std::pair<const Node *, bool> insert_or_assign(const key_type &key,
                                                   const value_type &val) noexcept {
        Node *t;
        /* update the value */
        if (this->searchUtil(key, &t)) {
            assert(t->key == key);
            auto res = std::make_pair(t, false);
            do {
                t->val = val;
                t = t->_above;
            } while (t);
            return res;
        }
        // if (!this->searchUtil(key, &t) && t->key == key && t != head[0]) {
        //     do {
        //         t->val = val;
        //         t = t->_above;
        //     } while (t);
        //     return std::make_pair(t, true);
        // }
        assert(t->key < key || (key == MIN_KEY && t == head[0]));
        this->insertUntil(key, val, t);
        return {t->_next, true};
    }

    // Returns: the target node if found, otherwise null.
    const Node *find(const key_type &key) const {
        Node *p;
        if (this->searchUtil(key, &p)) {
            return p;
        }
        return nullptr;
    }

    std::vector<kv_type> get_kv() const noexcept {
        std::vector<kv_type> res{};
        Node *p = head[0]->_next;
        Node *sentinel = tail[0];
        while (p != sentinel) {
            res.emplace_back(p->key, p->val);
            p = p->_next;
        }
        return res;
    }

    std::pair<key_type, key_type> get_range() const noexcept {
        if (head[0] == tail[0]) {
            return {1, 0};
        }
        return {head[0]->_next->key, tail[0]->_pre->key};
    }

private:
    static constexpr size_type MAX_LEVEL = std::numeric_limits<size_type>::max();
    static constexpr key_type MAX_KEY = std::numeric_limits<key_type>::max();
    static constexpr key_type MIN_KEY = std::numeric_limits<key_type>::min();
    // static constexpr size_type PR = 2; // The reciprocal of jump possibility.
    static constexpr size_type BOUND = 24108;  // Jump possibility: 1/e

    std::vector<Node *> head;
    std::vector<Node *> tail;
    int h;  // Max height.

    inline int randLevel() const noexcept {
        return SkipList::randLevel(BOUND, MAX_LEVEL);
    }
    inline static int randLevel(int upper, size_type levelMax) noexcept {
        size_type level = 0;
        while ((rand() & 0xFFFF) < upper) {
            ++level;
        }
        return level < levelMax ? level : levelMax;
    }
    /**
     * @brief A tool function to search for a node.
     *
     * @param key the target key.
     * @param p if returns true, p is the target node,
     *        otherwise the previous node where key should be inserted.
     * @return true if the key exists
     */
    bool searchUtil(const key_type &key, Node **p) const {
        *p = nullptr;

        /** Special cases: key == MIN_KEY or key == MAX_KEY */
        if (key == MIN_KEY) {
            if (head[0]->_next->key == MIN_KEY) {
                *p = head[0]->_next;
                return true;
            }
            *p = head[0];
            return false;
        }
        if (key == MAX_KEY) {
            *p = tail[0]->_pre;
            if (tail[0]->_pre->key == MAX_KEY) {
                return true;
            }
            return false;
        }
        /** The key isn't KEY_MIN neither MAX_KEY */
        Node *t = head[h];
        while (1) {
            assert(t && t->_next);
            while (t->_next->key <= key) {
                t = t->_next;
            }
            if (t->key == key) {
                while (t->_below)
                    t = t->_below;
                *p = t;
                return true;
            }
            if (!t->_below) {
                *p = t;
                return false;
            }
            t = t->_below;
        }
        assert(0);
        return false;
    }

    /**
     * @brief A tool function to insert a node.
     *        The function will always performs an insertion.
     *
     * @param key
     * @param val
     * @param t the previous node.
     */
    void insertUntil(const key_type &key, const value_type &val, Node *t) noexcept {
        /** At the right of t, a tower will be established. */
        Node *pRight = t->_next, *pLeft = t;
        int level = randLevel();
        pLeft->_next = new Node(key, val, pLeft, pRight);
        t = pLeft->_next;
        pRight->_pre = t;
        while (h < level) {
            ++h;
            head.push_back(new Node(MIN_KEY, value_type{}));
            tail.push_back(new Node(MAX_KEY, value_type{}));
            head[h]->_next = tail[h];
            tail[h]->_pre = head[h];
            head[h]->_below = head[h - 1];
            head[h - 1]->_above = head[h];
            tail[h - 1]->_above = tail[h];
            tail[h]->_below = tail[h - 1];
            tail[h - 1]->_above = tail[h];
        }
        while (level--) {
            while (!pLeft->_above) {
                pLeft = pLeft->_pre;
            }
            while (!pRight->_above) {
                pRight = pRight->_next;
            }
            pLeft = pLeft->_above;
            pRight = pRight->_above;
            t->_above = new Node(key, val, pLeft, pRight, nullptr, t);
            t = t->_above;
            pLeft->_next = t;
            pRight->_pre = t;
        }
    }
};
template <typename KeyT, typename ValT, typename _X>
constexpr size_type SkipList<KeyT, ValT, _X>::MAX_LEVEL;

template <typename KeyT, typename ValT, typename _X>
constexpr KeyT SkipList<KeyT, ValT, _X>::MAX_KEY;

template <typename KeyT, typename ValT, typename _X>
constexpr KeyT SkipList<KeyT, ValT, _X>::MIN_KEY;
}  // namespace basic_ds

#endif