#include "MemTable.h"

// #define PRINT

#ifdef PRINT
#include <iostream>
using std::cout;
using std::endl;
#endif

namespace mtb {
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
}