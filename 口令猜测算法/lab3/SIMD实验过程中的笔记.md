C/C++中的 `#define` 指令用于创建宏。宏本质上是一种文本替换机制，由预处理器在编译前执行。当您 `#define` 某物时，预处理器将替换定义的宏的每个出现，用其对应值或代码。

在提供的代码中， `FF` 、 `GG` 、 `HH` 和 `II` 被定义为宏，而不是函数。这就是为什么它们没有显式的 `return` 语句。它们不是返回值，而是直接使用 `+=` 和 `=` 操作符修改它们的参数（ `a` 、 `b` 、 `c` 、 `d` ）。这些更改直接应用于传递给宏的变量。这与函数有重要区别，因为函数通常在参数的副本上操作（除非你传递指针或引用）。

FF 宏定义了 MD5 算法中第一轮变换的步骤，其中：

- `a`, `b`, `c`, `d` 是 MD5 状态的四个 32 位字
- `x` 是输入消息的一个 32 位块
- `s` 指定左循环移位的位数
- `ac` 是一个基于正弦函数的常量

![image-20250413163736067](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413163736067.png)

得到了正确的结果

效率变低了，我们需要分析原因：

**内存分配和访问模式低效**：

- 为每个消息单独调用StringProcess，导致多次内存分配
- 从多个不连续内存位置加载数据，导致缓存利用率低

![image-20250413195904843](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413195904843.png)

训练时间太久了所以我们将训练数据减少到30000

![image-20250413200726960](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413200726960.png)

![image-20250413200908390](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413200908390.png)

重新选定基准值9.05s，8.94s，下面是并行化之后的结果（已经优化了访存模式

![image-20250413201006680](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413201006680.png)

![image-20250413202621227](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413202621227.png)

1. **Annotate MD5Hash_SIMD**
   - 显示MD5Hash_SIMD函数的源代码级性能注解
   - 可以看到每行代码执行的频率和耗时，帮助定位具体的性能瓶颈
2. **Zoom into main thread**
   - 只查看主线程的性能数据
   - 过滤掉其他线程的数据
3. **Zoom into main DSO**
   - 缩小视图到主要的动态共享对象
   - `k`快捷键可以直接查看内核相关性能数据
4. **Browse map details**
   - 浏览内存映射的详细信息
   - 查看代码和数据的内存分布
5. **Run scripts for samples of symbol [MD5Hash_SIMD]**
   - 对MD5Hash_SIMD函数的采样数据运行自定义脚本
   - 可以进行进一步的自动化分析
6. **Run scripts for all samples**
   - 对所有采样数据运行脚本
7. **Switch to another data file in PWD**
   - 切换到当前目录中的其他perf数据文件
8. **Exit**
   - 退出perf report工具

![image-20250413203253615](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413203253615.png)

汇编代码中与0相比较耗费了我们较多的时间，所以我们选择打开循环，直接执行循环里面的操作

```c++
for (int i = 0; i < 4; i++) {
    Byte* block_ptr = paddedMessages[i] + block * 64 + j * 4;
    // 使用位运算而不是多次左移减少指令
    values[i] = (uint32_t)block_ptr[0] | 
        ((uint32_t)block_ptr[1] << 8) |
        ((uint32_t)block_ptr[2] << 16) | 
        ((uint32_t)block_ptr[3] << 24);
}
```

```c++
// 展开的数据加载循环
Byte* block_ptr0 = paddedMessages[0] + block * 64 + j * 4;
Byte* block_ptr1 = paddedMessages[1] + block * 64 + j * 4;
Byte* block_ptr2 = paddedMessages[2] + block * 64 + j * 4;
Byte* block_ptr3 = paddedMessages[3] + block * 64 + j * 4;

values[0] = (uint32_t)block_ptr0[0] | ((uint32_t)block_ptr0[1] << 8) | 
((uint32_t)block_ptr0[2] << 16) | ((uint32_t)block_ptr0[3] << 24);
values[1] = (uint32_t)block_ptr1[0] | ((uint32_t)block_ptr1[1] << 8) | 
((uint32_t)block_ptr1[2] << 16) | ((uint32_t)block_ptr1[3] << 24);
values[2] = (uint32_t)block_ptr2[0] | ((uint32_t)block_ptr2[1] << 8) | 
((uint32_t)block_ptr2[2] << 16) | ((uint32_t)block_ptr2[3] << 24);
values[3] = (uint32_t)block_ptr3[0] | ((uint32_t)block_ptr3[1] << 8) | 
((uint32_t)block_ptr3[2] << 16) | ((uint32_t)block_ptr3[3] << 24);
```

```c++
paddedMessages[0] = buffer + 0 * max_padded_length;
PrepareMessage(inputs[0], paddedMessages[0], &messageLengths[0]);
paddedMessages[1] = buffer + 1 * max_padded_length;
PrepareMessage(inputs[1], paddedMessages[1], &messageLengths[1]);
paddedMessages[2] = buffer + 2 * max_padded_length;
PrepareMessage(inputs[2], paddedMessages[2], &messageLengths[2]);
paddedMessages[3] = buffer + 3 * max_padded_length;
PrepareMessage(inputs[3], paddedMessages[3], &messageLengths[3]);
```

![image-20250413203840408](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413203840408.png)

![image-20250413203931881](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413203931881.png)

![image-20250413204033626](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413204033626.png)

差不多优化了1秒，还是很多的

我们使用静态变量`a0_init`，减少每次进入函数时的vdupq_n_u32运算

![image-20250413204924353](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413204924353.png)

好像没什么变化

```c++
// 替换这样的循环:
max_length = std::max(max_length, inputs[0].length());
max_length = std::max(max_length, inputs[1].length());
max_length = std::max(max_length, inputs[2].length());
max_length = std::max(max_length, inputs[3].length());

// 使用条件移动代替条件分支
max_padding_bits = (max_padded_bits == 448) ? 512 : 
                  ((448 - max_padded_bits) & 0x1FF);
```

![image-20250413210721130](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413210721130.png)

还是没有什么优化感觉

```
const size_t block_offsets[16] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
```

再加一个静态变量，把这个的乘法也省掉了

```
Byte* block_ptr0 = paddedMessages[0] + block * 64 + block_offsets[j];
Byte* block_ptr1 = paddedMessages[1] + block * 64 + block_offsets[j];
Byte* block_ptr2 = paddedMessages[2] + block * 64 + block_offsets[j];
Byte* block_ptr3 = paddedMessages[3] + block * 64 + block_offsets[j];
```

![image-20250413211326563](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413211326563.png)

还是没变化，我们从头再来哈，这个是并行的结果19.46s

![image-20250413213002788](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413213002788.png)

13.78s

![image-20250413220650721](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413220650721.png)

![image-20250413221124729](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413221124729.png)

来个长图比对一下

![image-20250413221958141](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413221958141.png)

豪德我们选中了内联函数ROTATELEFT_SIMD,并将其变为宏定义，时间直接减少了1s

```c++
// ROTATELEFT的SIMD版本
inline bit32x4_t ROTATELEFT_SIMD(bit32x4_t x, int n) {
    return vorrq_u32(vshlq_n_u32(x, n), vshrq_n_u32(x, 32 - n));
}

#define ROTATELEFT_SIMD(num, n) (vorrq_u32(vshlq_n_u32((num), (n)), vshrq_n_u32((num), 32 - (n))))

```

![image-20250413222935435](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413222935435.png)

![image-20250413223036516](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413223036516.png)

ROTATELEFT也从高时间消失了

那我们不妨把这些也改为宏定义

```c++
// 添加SIMD版本的基本函数
inline bit32x4_t F_SIMD(bit32x4_t x, bit32x4_t y, bit32x4_t z) {
  return vorrq_u32(vandq_u32(x, y), vandq_u32(vmvnq_u32(x), z));
}

inline bit32x4_t G_SIMD(bit32x4_t x, bit32x4_t y, bit32x4_t z) {
  return vorrq_u32(vandq_u32(x, z), vandq_u32(y, vmvnq_u32(z)));
}

inline bit32x4_t H_SIMD(bit32x4_t x, bit32x4_t y, bit32x4_t z) {
  return veorq_u32(veorq_u32(x, y), z);
}

inline bit32x4_t I_SIMD(bit32x4_t x, bit32x4_t y, bit32x4_t z) {
  return veorq_u32(y, vorrq_u32(x, vmvnq_u32(z)));
}

#define F_SIMD(x, y, z) (vorrq_u32(vandq_u32((x), (y)), vandq_u32(vmvnq_u32((x)), (z))))
#define G_SIMD(x, y, z) (vorrq_u32(vandq_u32((x), (z)), vandq_u32((y), vmvnq_u32((z)))))
#define H_SIMD(x, y, z) (veorq_u32(veorq_u32((x), (y)), (z)))
#define I_SIMD(x, y, z) (veorq_u32((y), vorrq_u32((x), vmvnq_u32((z)))))
```

![image-20250413223521485](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413223521485.png)

时间也来到了17.4s

![image-20250413223547759](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250413223547759.png)

内联函数的时间消耗已经被消除了但是主函数的时间增加了，我们可以看出内联函数虽然可以减少主函数的时间，但自身会增加运行时间

我们观察就可以发现凹其他的时间已经没用了，只能去凹主函数

![image-20250414000545782](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414000545782.png)

看到这个没有，百分之4.05，优化他！

```c++
memset(paddedMessage + length + 1, 0, paddingBytes - 1); // 填充0字节是这个语句
```

```c++
static Byte zero_buffer[MAX_BUFFER_SIZE] = {0};//加上一个静态缓冲区
memcpy(output + input_length + 1, zero_buffer, padding_bytes - 1);//用拷贝复制信息
```

![image-20250414000936013](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414000936013.png)

![image-20250414001056921](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414001056921.png)

16.6s了已经

![image-20250414002953703](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414002953703.png)

性能瓶颈的汇编代码，我们再定位一下代码位置

```c++
Byte* buffer = new Byte[4 * max_padded_length]();  // 括号导致内存清零

Byte* buffer = new Byte[4 * max_padded_length];//只初始化必要的部分
```

![image-20250414003358182](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414003358182.png)

15.5s

![image-20250414004049666](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414004049666.png)

定位代码

```c++
// 更高效的数据加载
for (int j = 0; j < 16; j++) {
    // 一次性加载4个值
    uint32_t values[4];
    // 展开的数据加载循环
    Byte* block_ptr0 = paddedMessages[0] + block * 64 + j * 4;
    Byte* block_ptr1 = paddedMessages[1] + block * 64 + j * 4;
    Byte* block_ptr2 = paddedMessages[2] + block * 64 + j * 4;
    Byte* block_ptr3 = paddedMessages[3] + block * 64 + j * 4;

    values[0] = (uint32_t)block_ptr0[0] | ((uint32_t)block_ptr0[1] << 8) | ((uint32_t)block_ptr0[2] << 16) | ((uint32_t)block_ptr0[3] << 24);
    values[1] = (uint32_t)block_ptr1[0] | ((uint32_t)block_ptr1[1] << 8) | ((uint32_t)block_ptr1[2] << 16) | ((uint32_t)block_ptr1[3] << 24);
    values[2] = (uint32_t)block_ptr2[0] | ((uint32_t)block_ptr2[1] << 8) | ((uint32_t)block_ptr2[2] << 16) | ((uint32_t)block_ptr2[3] << 24);
    values[3] = (uint32_t)block_ptr3[0] | ((uint32_t)block_ptr3[1] << 8) | ((uint32_t)block_ptr3[2] << 16) | ((uint32_t)block_ptr3[3] << 24);
    // 使用NEON指令直接加载数据
    M[j] = vld1q_u32(values);
}
```

```c++
static const size_t BLOCK_OFFSETS[16] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
//加上静态变量避免重复计算

```

![image-20250414004338552](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414004338552.png)

似乎没有优化，现在最多的变为了这个

![image-20250414092616028](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414092616028.png)

![image-20250414112038884](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414112038884.png)

![image-20250414155303291](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414155303291.png)

指针偏移操作消耗了大量的时间

![image-20250414195631085](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414195631085.png)

```c++
// 从每个消息中提取当前块的数据
bit32x4_t M[16];  // 栈上分配的大型向量数组

for (int j = 0; j < 16; j++) {
    // ...计算values值...

    // 存储到栈上数组 - 这个操作生成了复杂的栈偏移计算
    M[j] = vld1q_u32(values);
}

// 预先声明一个静态对齐的数组
static bit32x4_t M_static[16] __attribute__((aligned(16)));
// 使用静态预分配数组替代栈上数组
bit32x4_t* M = M_static;
// 更高效的数据加载，使用指针消除栈偏移计算
for (int j = 0; j < 16; j++) {
    const size_t offset = block_base + BLOCK_OFFSETS[j];

    // 数据加载保持不变
    uint32_t values[4] __attribute__((aligned(16)));
    memcpy(&values[0], paddedMessages[0] + offset, 4);
    // ...复制其他值...

    // 使用预分配的静态数组存储
    M[j] = vld1q_u32(values);
}
```

没什么变化还是15.5左右，继续

![image-20250414195932279](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414195932279.png)

![image-20250414200018547](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250414200018547.png)