#ifndef MEMTABLE
#define MEMTABLE

#include <cassert>
#include <string>
#include <vector>
#include "types.h"

// #define PRINT

#ifdef PRINT
#include <iostream>
using std::cout;
using std::endl;
#endif

namespace mtb {

#define MOD 0xFFFF
#define EH 24108

class MemTable;

class SkipList {
    friend class MemTable;
    typedef struct SkipListNode {
        Key _key;
        Val _val;
        SkipListNode *pre, *next, *above, *below;
        SkipListNode(Key k = 0, Val v = VAL_INVALID, SkipListNode *p1 = nullptr,
                     SkipListNode *p2 = nullptr, SkipListNode *p3 = nullptr,
                     SkipListNode *p4 = nullptr)
            : _key(k), _val(v), pre(p1), next(p2), above(p3), below(p4) {}
    } Node;

public:
    SkipList(int pr = 2, int l = INT_MAX, bool e = false)
        : POSSIBILITY(pr), BOUND(e ? EH : MOD / pr), MAX_LEVEL(l), h(0) {
        std::srand(std::time(nullptr));
        head.emplace_back(new Node(KEY_MIN));
        tail.emplace_back(new Node(KEY_MAX));
        head[0]->next = tail[0];
        tail[0]->pre = head[0];
    }

    SkipList(const SkipList &) = delete;
    SkipList(SkipList &&) = delete;

    ~SkipList() {
        for (int i = 0; i <= h; ++i) {
            Node *p = head[i];
            do {
#ifdef PRINT
                cout << p->_key << '\t';
#endif
                p = p->next;
                delete p->pre;
            } while (p->next);
            delete p;
#ifdef PRINT
            cout << endl;
#endif
        }
    }

    static int randLevel(int upper, int levelMax) {
        int level = 0;
        while ((rand() & MOD) < upper) {
            ++level;
        }
        return level < levelMax ? level : levelMax;
    }

    int randLevel() const {
        return SkipList::randLevel(BOUND, MAX_LEVEL);
    }

    Val search(Key key) const {
        Node *p;
        if (searchUtil(key, &p)) {
            return p->_val;
        }
        return VAL_INVALID;
    }

    bool insert(const Key& key, const Val& val, bool update = true) {
        /* Never insert VAL_INVALID */
        if (val == VAL_INVALID)
            return false;
        Node *t;
        if (searchUtil(key, &t) && !update) {
            return false;
        }
        /* update the value */
        if (t->_key == key && t != head[0]) {
            do {
                t->_val = val;
                t = t->above;
            } while (t);
            return true;
        }

        assert(t->_key < key || (key == KEY_MIN && t == head[0]));
        insertUntil(key, val, t);
        return true;
    }

private:
    const int POSSIBILITY;
    const int BOUND;
    const int MAX_LEVEL;
    std::vector<Node *> head;
    std::vector<Node *> tail;
    int h;

    bool searchUtil(const Key& key, Node **p) const {
        *p = nullptr;
        if (key == KEY_MIN) {
            if (head[0]->next->_key == KEY_MIN) {
                *p = head[0]->next;
                return true;
            }
            *p = head[0];
            return false;
        }
        if (key == KEY_MAX) {
            *p = tail[0]->pre;
            if (tail[0]->pre->_key == KEY_MAX) {
                return true;
            }
            return false;
        }
        /* key isn't KEY_MIN neither KEY_MAX */
        Node *t = head[h];
        while (1) {
            assert(t && t->next);
            while (t->next->_key <= key) {
                t = t->next;
            }
            if (t->_key == key) {
                while (t->below)
                    t = t->below;
                *p = t;
                return true;
            }
            if (!t->below) {
                *p = t;
                return false;
            }
            t = t->below;
        }
        assert(0);
        return false;
    }

    void insertUntil(const Key &key, const Val &val, Node* t) {
        /* at the right of t, a tower will be established */
        Node *pRight = t->next, *pLeft = t;
        int level = randLevel();
        pLeft->next = new Node(key, val, pLeft, pRight);
        t = pLeft->next;
        pRight->pre = t;
        while (h < level) {
            ++h;
            head.emplace_back(new Node(KEY_MIN));
            tail.emplace_back(new Node(KEY_MAX));
            head[h]->next = tail[h];
            tail[h]->pre = head[h];
            head[h]->below = head[h - 1];
            head[h - 1]->above = head[h];
            tail[h - 1]->above = tail[h];
            tail[h]->below = tail[h - 1];
            tail[h - 1]->above = tail[h];
        }
        while (level--) {
            while (!pLeft->above) {
                pLeft = pLeft->pre;
            }
            while (!pRight->above) {
                pRight = pRight->next;
            }
            pLeft = pLeft->above;
            pRight = pRight->above;
            t->above = new Node(key, val, pLeft, pRight, nullptr, t);
            t = t->above;
            pLeft->next = t;
            pRight->pre = t;
        }
    }
};

class MemTable {
public:
    /**
     * @param mem_max The max memory of SSTabel
     * @param x The reciprocal of possibility, used in skip list
     */
    MemTable(int mem_max = 2, int x = 4) : MEM_MAX(mem_max), sl(new SkipList(4)) {}

    void put(Key k, Val v) {
        if (!available(k, v)) {
            toSST();
        }
        sl->insert(k, v, true);
        /**
         * @todo update memory size
         *
         */
    }

    /**
     * @brief Search if there are key in the memory table
     *
     * @return true: it's found, parameter v is set as the value
     * @return false: there are no key in memory table, v isn't defined
     */
    bool get(Key k, Val* v) {
        SkipList::Node *p;
        if (sl->searchUtil(k, &p)) {
            *v = p->_val;
            return true;
        }
        return false;
    }

protected:
    /**
     * @brief try to remove a key (if it exists in the memory table)
     *
     * @return true: removed, p is not defined
     *         false: the key not exists in the memory table
     *                p is set to the previous
     */
    bool removeTry(Key k, SkipList::Node** p) {
        // if (!sl->searchUtil(k, p))
        //     return false;
        // SkipList::Node* tmp;
        // do {
        //     tmp = p->above;
        //     p->pre->next = p->next;
        //     delete p;
        //     p = tmp;
        // } while (p);
        return true;
    }

    bool remove(SkipList::Node* p) {
        return true;
    }

private:
    SkipList *sl;
    const int MEM_MAX;
    int mem; // always <= MAX_MEMORY

    /**
     * @brief Check whether there is enough space
     *        **after** put (k, v)
     */
    bool available(Key k, Val v) const {
        /**
         * @todo
         */
        return true;
    }

    void toSST() {
        /**
         * @todo
         *
         */
        int p = sl->POSSIBILITY;
        delete sl;
        sl = new SkipList(p);
    }
};

#undef MOD
#undef EH

}  // namespace mtb

#endif