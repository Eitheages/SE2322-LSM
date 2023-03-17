#ifndef MEMTABLE
#define MEMTABLE

#include <cassert>
#include <string>
#include <vector>
#include "types.h"

namespace mtb {

class MemTable;

class SkipList {
    friend class MemTable;
    struct SkipListNode;
    typedef struct SkipListNode Node;

public:
    SkipList(int pr = 2, int l = INT_MAX, bool e = false);

    SkipList(const SkipList &) = delete;
    SkipList(SkipList &&) = delete;

    ~SkipList();

    inline static int randLevel(int upper, int levelMax) {
        int level = 0;
        while ((rand() & 0xFFFF) < upper) {
            ++level;
        }
        return level < levelMax ? level : levelMax;
    }

    inline int randLevel() const {
        return SkipList::randLevel(BOUND, MAX_LEVEL);
    }

    Val search(const Key &key) const;

    bool insert(const Key &key, const Val &val, bool merge = true);

private:
    const int POSSIBILITY;
    const int BOUND;
    const int MAX_LEVEL;
    std::vector<Node *> head;
    std::vector<Node *> tail;
    int h;

    bool searchUtil(const Key &key, Node **p) const;
    void insertUntil(const Key &key, const Val &val, Node *t);
};

// class MemTable {
// public:
//     /**
//      * @param mem_max The max memory of SSTabel
//      * @param x The reciprocal of possibility, used in skip list
//      */
//     MemTable(int mem_max = MAX_MEMORY, int p = 4)
//         : MEM_MAX(mem_max), sl(new SkipList(p)) {}

//     void put(Key k, Val v) {
//         if (!available(k, v)) {
//             toSST();
//         }
//         sl->insert(k, v, true);
//         /**
//          * @todo update memory size
//          *
//          */
//     }

//     /**
//      * @brief Search if there are key in the memory table
//      *
//      * @return true: it's found, parameter v is set as the value
//      * @return false: there are no key in memory table, v isn't defined
//      */
//     bool get(Key k, Val *v) {
//         SkipList::Node *p;
//         if (sl->searchUtil(k, &p)) {
//             *v = p->_val;
//             return true;
//         }
//         return false;
//     }

// protected:
//     /**
//      * @brief try to remove a key (if it exists in the memory table)
//      *
//      * @return true: removed, p is not defined
//      *         false: the key not exists in the memory table
//      *                p is set to the previous
//      */
//     bool removeTry(Key k, SkipList::Node **p) {
//         // if (!sl->searchUtil(k, p))
//         //     return false;
//         // SkipList::Node* tmp;
//         // do {
//         //     tmp = p->above;
//         //     p->pre->next = p->next;
//         //     delete p;
//         //     p = tmp;
//         // } while (p);
//         return true;
//     }

//     bool remove(SkipList::Node *p) {
//         return true;
//     }

// private:
//     const int MEM_MAX;
//     SkipList *sl;
//     int mem;  // always <= MAX_MEMORY

//     /**
//      * @brief Check whether there is enough space
//      *        **after** put (k, v)
//      */
//     bool available(Key k, Val v) const {
//         /**
//          * @todo
//          */
//         return true;
//     }

//     void toSST() {
//         /**
//          * @todo
//          *
//          */
//         int p = sl->POSSIBILITY;
//         delete sl;
//         sl = new SkipList(p);
//     }
// };

}  // namespace mtb

#endif