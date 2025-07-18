 在C语言中，NULL是一个宏定义，通常被称为空指针，代表一个特殊的指针值
 C语言中复制字符串需要区分复制指针和复制内容，默认的赋值操作仅复制指针
 isdigit检查是否为数字
 islower 小写

研究双向和单向链表，以及什么情况下其中一种优于另一种。
研究双向链表的限制。例如，虽然它们对于插入和删除元素很高效，但是对于变量元素比较慢。
还缺少什么你能想到的操作？比如复制、连接、分割等等。实现这些操作，并且为它们编写单元测试。

要理解单向链表和双向链表的差异及适用场景，我们先从结构定义入手，再分析核心特性、适用场景、限制，最后实现关键操作并编写测试。
### 一、单向链表与双向链表的结构定义
#### 1. 单向链表（Singly Linked List）
每个节点仅包含“数据”和“指向下一节点的指针”，只能**从头部向尾部遍历**。
```c
// 单向链表节点
typedef struct SinglyNode {
    int data;
    struct SinglyNode* next;  // 仅指向下一节点
} SinglyNode;

// 单向链表（记录头节点和长度）
typedef struct SinglyList {
    SinglyNode* head;
    int length;
} SinglyList;
```

#### 2. 双向链表（Doubly Linked List）
每个节点包含“数据”“指向下一节点的指针”“指向前一节点的指针”，**可双向遍历**。
```c
// 双向链表节点
typedef struct DoublyNode {
    int data;
    struct DoublyNode* prev;  // 指向前一节点
    struct DoublyNode* next;  // 指向下一节点
} DoublyNode;

// 双向链表（记录头、尾节点和长度，优化操作效率）
typedef struct DoublyList {
    DoublyNode* head;
    DoublyNode* tail;
    int length;
} DoublyList;
```


### 二、核心特性对比
| 特性                | 单向链表（Singly）                          | 双向链表（Doubly）                          |
|---------------------|--------------------------------------------|--------------------------------------------|
| **内存占用**        | 低（每个节点1个指针）                       | 高（每个节点2个指针，约多50%内存）          |
| **遍历方向**        | 仅正向（头→尾）                             | 双向（头→尾、尾→头）                       |
| **插入操作**        | 已知前驱节点时O(1)；否则需遍历找前驱（O(n)） | 已知节点时O(1)（无需找前驱，直接修改prev）  |
| **删除操作**        | 需遍历找前驱节点（O(n)）                    | 已知节点时O(1)（通过prev直接定位前驱）      |
| **实现复杂度**      | 简单（仅维护next指针）                      | 复杂（需同时维护prev和next，易出错）        |
| **适用场景**        | 内存受限、仅需正向遍历（如单链表栈、队列）   | 需要双向遍历、频繁增删（如浏览器历史、LRU缓存） |


### 三、双向链表的限制
1. **内存开销更高**：每个节点多一个`prev`指针，对内存敏感场景（如嵌入式设备）不友好。
2. **操作复杂度高**：插入/删除时需同时更新`prev`和`next`，若逻辑错误易导致链表断裂（如漏改`prev`导致反向遍历失败）。
3. **缓存利用率低**：节点数据和两个指针分散存储，CPU缓存加载效率低于单向链表（数据和单个指针更紧凑）。


### 四、关键操作实现（复制、连接、分割）
以下实现单向和双向链表的核心操作，并对比差异。


#### 1. 基础工具函数（创建节点、初始化链表）
```c
// 单向节点创建
SinglyNode* singly_create_node(int data) {
    SinglyNode* node = malloc(sizeof(SinglyNode));
    node->data = data;
    node->next = NULL;
    return node;
}

// 双向节点创建
DoublyNode* doubly_create_node(int data) {
    DoublyNode* node = malloc(sizeof(DoublyNode));
    node->data = data;
    node->prev = node->next = NULL;
    return node;
}

// 初始化链表
SinglyList* singly_init() {
    SinglyList* list = malloc(sizeof(SinglyList));
    list->head = NULL;
    list->length = 0;
    return list;
}

DoublyList* doubly_init() {
    DoublyList* list = malloc(sizeof(DoublyList));
    list->head = list->tail = NULL;
    list->length = 0;
    return list;
}
```


#### 2. 复制操作（深拷贝链表）
复制需创建新节点，避免原链表修改影响副本。

##### 单向链表复制
```c
SinglyList* singly_copy(SinglyList* src) {
    if (!src) return NULL;
    SinglyList* dest = singly_init();
    SinglyNode* cur_src = src->head;
    SinglyNode* cur_dest = NULL;

    while (cur_src) {
        SinglyNode* new_node = singly_create_node(cur_src->data);
        if (dest->head == NULL) {  // 初始化头节点
            dest->head = new_node;
            cur_dest = new_node;
        } else {
            cur_dest->next = new_node;  // 链接新节点
            cur_dest = new_node;
        }
        cur_src = cur_src->next;
        dest->length++;
    }
    return dest;
}
```

##### 双向链表复制（需同步复制prev指针）
```c
DoublyList* doubly_copy(DoublyList* src) {
    if (!src) return NULL;
    DoublyList* dest = doubly_init();
    DoublyNode* cur_src = src->head;
    DoublyNode* cur_dest = NULL;

    while (cur_src) {
        DoublyNode* new_node = doubly_create_node(cur_src->data);
        if (dest->head == NULL) {  // 初始化头节点
            dest->head = new_node;
            dest->tail = new_node;
            cur_dest = new_node;
        } else {
            cur_dest->next = new_node;  // 链接next
            new_node->prev = cur_dest;  // 链接prev（双向链表核心）
            cur_dest = new_node;
            dest->tail = new_node;  // 更新尾节点
        }
        cur_src = cur_src->next;
        dest->length++;
    }
    return dest;
}
```


#### 3. 连接操作（合并两个链表）
将链表B连接到链表A末尾，A和B需为独立链表（避免循环引用）。

##### 单向链表连接
```c
void singly_concat(SinglyList* a, SinglyList* b) {
    if (!a || !b || b->length == 0) return;
    if (a->length == 0) {  // A为空，直接指向B的头
        a->head = b->head;
    } else {  // 找到A的尾节点，链接到B的头
        SinglyNode* cur = a->head;
        while (cur->next) cur = cur->next;  // 遍历找尾（O(n)，单向链表的劣势）
        cur->next = b->head;
    }
    a->length += b->length;
    // 清空B（避免重复释放）
    b->head = NULL;
    b->length = 0;
}
```

##### 双向链表连接（利用tail节点优化，O(1)）
```c
void doubly_concat(DoublyList* a, DoublyList* b) {
    if (!a || !b || b->length == 0) return;
    if (a->length == 0) {  // A为空，直接复用B的头和尾
        a->head = b->head;
        a->tail = b->tail;
    } else {  // 直接链接A的尾和B的头（双向）
        a->tail->next = b->head;    // A尾的next → B头
        b->head->prev = a->tail;    // B头的prev → A尾（双向核心）
        a->tail = b->tail;          // 更新A的尾为B的尾
    }
    a->length += b->length;
    // 清空B
    b->head = b->tail = NULL;
    b->length = 0;
}
```


#### 4. 分割操作（按值拆分链表）
以`key`为界，将链表拆分为“小于等于key”和“大于key”两个链表。

##### 单向链表分割（需维护4个指针：两个新链表的头和尾）
```c
void singly_split(SinglyList* src, int key, SinglyList* left, SinglyList* right) {
    if (!src || !left || !right) return;
    SinglyNode *left_head = NULL, *left_tail = NULL;
    SinglyNode *right_head = NULL, *right_tail = NULL;
    SinglyNode* cur = src->head;

    while (cur) {
        SinglyNode* next = cur->next;  // 暂存next（避免拆分后丢失）
        cur->next = NULL;               // 断开原链接

        if (cur->data <= key) {  // 加入左链表
            if (!left_head) {
                left_head = cur;
                left_tail = cur;
            } else {
                left_tail->next = cur;
                left_tail = cur;
            }
            left->length++;
        } else {  // 加入右链表
            if (!right_head) {
                right_head = cur;
                right_tail = cur;
            } else {
                right_tail->next = cur;
                right_tail = cur;
            }
            right->length++;
        }
        cur = next;
    }
    left->head = left_head;
    right->head = right_head;
    // 清空原链表
    src->head = NULL;
    src->length = 0;
}
```

##### 双向链表分割（需同时维护prev指针）
```c
void doubly_split(DoublyList* src, int key, DoublyList* left, DoublyList* right) {
    if (!src || !left || !right) return;
    DoublyNode *left_head = NULL, *left_tail = NULL;
    DoublyNode *right_head = NULL, *right_tail = NULL;
    DoublyNode* cur = src->head;

    while (cur) {
        DoublyNode* next = cur->next;  // 暂存next
        cur->prev = cur->next = NULL;  // 断开原链接

        if (cur->data <= key) {  // 加入左链表
            if (!left_head) {
                left_head = cur;
                left_tail = cur;
            } else {
                left_tail->next = cur;  // 左尾next → 当前节点
                cur->prev = left_tail;  // 当前节点prev → 左尾
                left_tail = cur;
            }
            left->length++;
        } else {  // 加入右链表
            if (!right_head) {
                right_head = cur;
                right_tail = cur;
            } else {
                right_tail->next = cur;  // 右尾next → 当前节点
                cur->prev = right_tail;  // 当前节点prev → 右尾
                right_tail = cur;
            }
            right->length++;
        }
        cur = next;
    }
    // 更新左链表头和尾
    left->head = left_head;
    left->tail = left_tail;
    // 更新右链表头和尾
    right->head = right_head;
    right->tail = right_tail;
    // 清空原链表
    src->head = src->tail = NULL;
    src->length = 0;
}
```


### 五、单元测试（验证操作正确性）
测试逻辑：创建链表→执行操作→打印结果→验证长度和数据是否符合预期。

```c
// 打印单向链表
void singly_print(SinglyList* list, const char* name) {
    printf("%s: ", name);
    SinglyNode* cur = list->head;
    while (cur) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("(length: %d)\n", list->length);
}

// 打印双向链表（正向）
void doubly_print(DoublyList* list, const char* name) {
    printf("%s: ", name);
    DoublyNode* cur = list->head;
    while (cur) {
        printf("%d ", cur->data);
        cur = cur->next;
    }
    printf("(length: %d)\n", list->length);
}

// 测试函数
int main() {
    // 1. 单向链表测试
    printf("=== 单向链表测试 ===\n");
    SinglyList* s1 = singly_init();
    s1->head = singly_create_node(1);
    s1->head->next = singly_create_node(3);
    s1->length = 2;
    singly_print(s1, "s1初始");  // 预期：1 3 (length: 2)

    SinglyList* s2 = singly_init();
    s2->head = singly_create_node(5);
    s2->length = 1;
    singly_print(s2, "s2初始");  // 预期：5 (length: 1)

    // 复制s1
    SinglyList* s1_copy = singly_copy(s1);
    singly_print(s1_copy, "s1复制");  // 预期：1 3 (length: 2)

    // 连接s1和s2
    singly_concat(s1, s2);
    singly_print(s1, "s1连接s2后");  // 预期：1 3 5 (length: 3)
    singly_print(s2, "s2连接后（空）");  // 预期：(length: 0)

    // 分割s1（按key=3）
    SinglyList* s_left = singly_init();
    SinglyList* s_right = singly_init();
    singly_split(s1, 3, s_left, s_right);
    singly_print(s_left, "分割左（<=3）");  // 预期：1 3 (length: 2)
    singly_print(s_right, "分割右（>3）");  // 预期：5 (length: 1)


    // 2. 双向链表测试
    printf("\n=== 双向链表测试 ===\n");
    DoublyList* d1 = doubly_init();
    d1->head = doubly_create_node(2);
    d1->head->next = doubly_create_node(4);
    d1->head->next->prev = d1->head;  // 手动链接prev
    d1->tail = d1->head->next;
    d1->length = 2;
    doubly_print(d1, "d1初始");  // 预期：2 4 (length: 2)

    // 复制d1
    DoublyList* d1_copy = doubly_copy(d1);
    doubly_print(d1_copy, "d1复制");  // 预期：2 4 (length: 2)

    DoublyList* d2 = doubly_init();
    d2->head = doubly_create_node(6);
    d2->tail = d2->head;
    d2->length = 1;
    doubly_print(d2, "d2初始");  // 预期：6 (length: 1)

    // 连接d1和d2
    doubly_concat(d1, d2);
    doubly_print(d1, "d1连接d2后");  // 预期：2 4 6 (length: 3)

    // 分割d1（按key=4）
    DoublyList* d_left = doubly_init();
    DoublyList* d_right = doubly_init();
    doubly_split(d1, 4, d_left, d_right);
    doubly_print(d_left, "分割左（<=4）");  // 预期：2 4 (length: 2)
    doubly_print(d_right, "分割右（>4）");  // 预期：6 (length: 1)

    return 0;
}
```


### 六、测试结果预期
```
=== 单向链表测试 ===
s1初始: 1 3 (length: 2)
s2初始: 5 (length: 1)
s1复制: 1 3 (length: 2)
s1连接s2后: 1 3 5 (length: 3)
s2连接后（空）:  (length: 0)
分割左（<=3）: 1 3 (length: 2)
分割右（>3）: 5 (length: 1)

=== 双向链表测试 ===
d1初始: 2 4 (length: 2)
d1复制: 2 4 (length: 2)
d2初始: 6 (length: 1)
d1连接d2后: 2 4 6 (length: 3)
分割左（<=4）: 2 4 (length: 2)
分割右（>4）: 6 (length: 1)
```


### 七、总结
- **单向链表**：适合内存受限、操作简单的场景，核心优势是内存紧凑、实现简单。
- **双向链表**：适合需要双向遍历、频繁增删的场景，核心优势是操作效率高（尤其是已知节点的增删）。
- **操作选择**：复制、连接、分割在双向链表中实现更复杂，但连接操作因`tail`指针优化可达到O(1)，单向链表需遍历找尾（O(n)）。

实际开发中，需根据内存、操作频率、遍历需求选择：简单场景用单向，复杂场景（如双向遍历）用双向。
