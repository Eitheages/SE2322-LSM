## MemTable.h

在命名空间 `mtb` 中封装了 MemTable 和 SkipList 类，其中 MemTable 类被声明为 SkipList 类的友元。这两个类的耦合度比较高，但是 SkipList 可以被单独使用。

SkipList 类维护四联表。其中，跳表的头部和尾部是始终存在，且从不修改的，令为哑结点。为了方便查找，跳表头部的结点 key 值为 `KEY_MIN`，尾部的结点 key 值为 `KEY_MAX`。极端情况下，插入键值对的键可能为 `KEY_MIN` 或 `KEY_MAX`。SkipList 类对这种情况进行了特判，确保了插入和查找操作的正确性。

#### APIs

下面两个是 SkipList 中比较重要的两个方法，它们被声明为私有成员，但是将在友元类 MemTable 中被使用。

+ `bool SkipList::searchUtil(const Key& k, Node** p)`：尝试找到键为 `k` 的元素，如果找到，返回 `true`，`p` 被设为找到的**最底层**结点；否则返回 `false`，同时 `p` 被设为即将被插入位置的前驱结点。

+ `void insertUntil(const Key &key, const Val &val, Node* t)`：在 `t` 结点的右侧插入键值对 `(key, val)`，层数由方法 `randLevel` 随机决定。该方法中不进行插入合法性的判断，所以调用时需要格外小心。

下面的方法是 SkipList 中的公有成员，它们主要被用于调试（或者单独使用 SkipList 类），没有在 MemTable 的实现中使用。

+ `Val search(const Key& key)`：查找键为 `key` 的元素，并返回它的值。如果没有键为 `key` 的元素，则返回 `VAL_INVALID`。SkipList 的实现保证 `VAL_INVALID` 不会是跳表中的的一个合法键值对的值。

+ `bool insert(const Key& key, const Val& val, bool update = true)`：插入键值对 `(key, val)`，返回是否成功插入。如果 `update == true`，则当存在键时，尝试更新值。返回 `false` 可能有两种情况：(a) 待插入键值对的值刚好就是 `VAL_INVALID`，SkipList 会阻止这种巧合；(b) 跳表中已经存在键为 `key` 的结点，而且 `update` 参数被置为 `false`。

## BloomFilter.h

这是一个比较危险的头文件，我不希望任何文件随意地包含它。为了提高执行效率，我将过滤器本身（一个 `std::bitset` 或 `std::vector<bool>` 对象）声明为一个外部变量（但是声明为内部链接，避免其他翻译单元调用），并且将大部分方法定义为宏。

文件中封装了一个实现 I/O 的类：BitBuffer。BitBuffer 是一个相对封装完好的类，并且重载了输入和输出流运算。颇需注意的是，输入和输出的过程中，我们几乎不进行错误检查，因此，要格外小心，否则程序很可能会朝着不可预料的方向发展。

+ `BitBuffer<N>::add(const std::bitset<N> &bs)`：将一个 bitset 对象存储在 buffer 中，便于后续输出。初始化构造时，如果传入 bitset 对象，这个方法也会被调用。

## Reference

[Easy bitset I/O](https://codereview.stackexchange.com/questions/67012/easy-bitset-i-o)