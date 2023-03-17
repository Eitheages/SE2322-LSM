#include "../include/MemTable.h"

// #define PRINT
#define NDEBUG

#ifdef PRINT
#include <iostream>
using std::cout;
using std::endl;
#endif

namespace mtb {

struct SkipList::SkipListNode {
    Key _key;
    Val _val;
    SkipListNode *pre, *next, *above, *below;
    SkipListNode(const Key &k = 0, const Val &v = VAL_INVALID, SkipListNode *p1 = nullptr,
                 SkipListNode *p2 = nullptr, SkipListNode *p3 = nullptr,
                 SkipListNode *p4 = nullptr)
        : _key(k), _val(v), pre(p1), next(p2), above(p3), below(p4) {}
};

SkipList::SkipList(int pr, int l, bool e)
    : POSSIBILITY(pr), BOUND(e ? 24108 : 0xFFFF / pr), MAX_LEVEL(l), h(0) {
    std::srand(std::time(nullptr));
    head.emplace_back(new Node(KEY_MIN));
    tail.emplace_back(new Node(KEY_MAX));
    head[0]->next = tail[0];
    tail[0]->pre = head[0];
}

SkipList::~SkipList() {
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

Val SkipList::search(const Key &key) const {
    Node *p;
    if (searchUtil(key, &p)) {
        return p->_val;
    }
    return VAL_INVALID;
}

void SkipList::insertUntil(const Key &key, const Val &val, Node *t) {
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

bool SkipList::insert(const Key &key, const Val &val, bool merge/*  = true */) {
    /* Never insert VAL_INVALID */
    if (val == VAL_INVALID)
        return false;
    Node *t;
    if (searchUtil(key, &t) && !merge) {
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

bool SkipList::searchUtil(const Key &key, Node **p) const {
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
}  // namespace mtb