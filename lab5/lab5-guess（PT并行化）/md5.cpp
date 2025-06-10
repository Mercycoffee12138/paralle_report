#include "md5.h"
#include <iomanip>
#include <assert.h>
#include <chrono>
#include <algorithm>
#include <thread> 

using namespace std;
using namespace chrono;

/**
 * StringProcess: 将单个输入字符串转换成MD5计算所需的消息数组
 * @param input 输入
 * @param[out] n_byte 用于给调用者传递额外的返回值，即最终Byte数组的长度
 * @return Byte消息数组
 */
Byte *StringProcess(string input, int *n_byte)
{
	// 将输入的字符串转换为Byte为单位的数组
	Byte *blocks = (Byte *)input.c_str();
	int length = input.length();

	// 计算原始消息长度（以比特为单位）
	int bitLength = length * 8;

	// paddingBits: 原始消息需要的padding长度（以bit为单位）
	// 对于给定的消息，将其补齐至length%512==448为止
	// 需要注意的是，即便给定的消息满足length%512==448，也需要再pad 512bits
	int paddingBits = bitLength % 512;
	if (paddingBits > 448)
	{
		paddingBits = 512 - (paddingBits - 448);
	}
	else if (paddingBits < 448)
	{
		paddingBits = 448 - paddingBits;
	}
	else if (paddingBits == 448)
	{
		paddingBits = 512;
	}

	// 原始消息需要的padding长度（以Byte为单位）
	int paddingBytes = paddingBits / 8;
	// 创建最终的字节数组
	// length + paddingBytes + 8:
	// 1. length为原始消息的长度（bits）
	// 2. paddingBytes为原始消息需要的padding长度（Bytes）
	// 3. 在pad到length%512==448之后，需要额外附加64bits的原始消息长度，即8个bytes
	int paddedLength = length + paddingBytes + 8;
	Byte *paddedMessage = new Byte[paddedLength];

	// 复制原始消息
	memcpy(paddedMessage, blocks, length);

	// 添加填充字节。填充时，第一位为1，后面的所有位均为0。
	// 所以第一个byte是0x80
	paddedMessage[length] = 0x80;							 // 添加一个0x80字节
	memset(paddedMessage + length + 1, 0, paddingBytes - 1); // 填充0字节

	// 添加消息长度（64比特，小端格式）
	for (int i = 0; i < 8; ++i)
	{
		// 特别注意此处应当将bitLength转换为uint64_t
		// 这里的length是原始消息的长度
		paddedMessage[length + paddingBytes + i] = ((uint64_t)length * 8 >> (i * 8)) & 0xFF;
	}

	// 验证长度是否满足要求。此时长度应当是512bit的倍数
	int residual = 8 * paddedLength % 512;
	// assert(residual == 0);

	// 在填充+添加长度之后，消息被分为n_blocks个512bit的部分
	*n_byte = paddedLength;
	return paddedMessage;
}


/**
 * MD5Hash: 将单个输入字符串转换成MD5
 * @param input 输入
 * @param[out] state 用于给调用者传递额外的返回值，即最终的缓冲区，也就是MD5的结果
 * @return Byte消息数组
 */
void MD5Hash(string input, bit32 *state)
{

	Byte *paddedMessage;
	int *messageLength = new int[1];
	for (int i = 0; i < 1; i += 1)
	{
		paddedMessage = StringProcess(input, &messageLength[i]);
		// cout<<messageLength[i]<<endl;
		assert(messageLength[i] == messageLength[0]);
	}
	int n_blocks = messageLength[0] / 64;

	// bit32* state= new bit32[4];
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;

	// 逐block地更新state
	for (int i = 0; i < n_blocks; i += 1)
	{
		bit32 x[16];

		// 下面的处理，在理解上较为复杂
		for (int i1 = 0; i1 < 16; ++i1)
		{
			x[i1] = (paddedMessage[4 * i1 + i * 64]) |
					(paddedMessage[4 * i1 + 1 + i * 64] << 8) |
					(paddedMessage[4 * i1 + 2 + i * 64] << 16) |
					(paddedMessage[4 * i1 + 3 + i * 64] << 24);
		}

		bit32 a = state[0], b = state[1], c = state[2], d = state[3];

		auto start = system_clock::now();
		/* Round 1 */
		FF(a, b, c, d, x[0], s11, 0xd76aa478);
		FF(d, a, b, c, x[1], s12, 0xe8c7b756);
		FF(c, d, a, b, x[2], s13, 0x242070db);
		FF(b, c, d, a, x[3], s14, 0xc1bdceee);
		FF(a, b, c, d, x[4], s11, 0xf57c0faf);
		FF(d, a, b, c, x[5], s12, 0x4787c62a);
		FF(c, d, a, b, x[6], s13, 0xa8304613);
		FF(b, c, d, a, x[7], s14, 0xfd469501);
		FF(a, b, c, d, x[8], s11, 0x698098d8);
		FF(d, a, b, c, x[9], s12, 0x8b44f7af);
		FF(c, d, a, b, x[10], s13, 0xffff5bb1);
		FF(b, c, d, a, x[11], s14, 0x895cd7be);
		FF(a, b, c, d, x[12], s11, 0x6b901122);
		FF(d, a, b, c, x[13], s12, 0xfd987193);
		FF(c, d, a, b, x[14], s13, 0xa679438e);
		FF(b, c, d, a, x[15], s14, 0x49b40821);

		/* Round 2 */
		GG(a, b, c, d, x[1], s21, 0xf61e2562);
		GG(d, a, b, c, x[6], s22, 0xc040b340);
		GG(c, d, a, b, x[11], s23, 0x265e5a51);
		GG(b, c, d, a, x[0], s24, 0xe9b6c7aa);
		GG(a, b, c, d, x[5], s21, 0xd62f105d);
		GG(d, a, b, c, x[10], s22, 0x2441453);
		GG(c, d, a, b, x[15], s23, 0xd8a1e681);
		GG(b, c, d, a, x[4], s24, 0xe7d3fbc8);
		GG(a, b, c, d, x[9], s21, 0x21e1cde6);
		GG(d, a, b, c, x[14], s22, 0xc33707d6);
		GG(c, d, a, b, x[3], s23, 0xf4d50d87);
		GG(b, c, d, a, x[8], s24, 0x455a14ed);
		GG(a, b, c, d, x[13], s21, 0xa9e3e905);
		GG(d, a, b, c, x[2], s22, 0xfcefa3f8);
		GG(c, d, a, b, x[7], s23, 0x676f02d9);
		GG(b, c, d, a, x[12], s24, 0x8d2a4c8a);

		/* Round 3 */
		HH(a, b, c, d, x[5], s31, 0xfffa3942);
		HH(d, a, b, c, x[8], s32, 0x8771f681);
		HH(c, d, a, b, x[11], s33, 0x6d9d6122);
		HH(b, c, d, a, x[14], s34, 0xfde5380c);
		HH(a, b, c, d, x[1], s31, 0xa4beea44);
		HH(d, a, b, c, x[4], s32, 0x4bdecfa9);
		HH(c, d, a, b, x[7], s33, 0xf6bb4b60);
		HH(b, c, d, a, x[10], s34, 0xbebfbc70);
		HH(a, b, c, d, x[13], s31, 0x289b7ec6);
		HH(d, a, b, c, x[0], s32, 0xeaa127fa);
		HH(c, d, a, b, x[3], s33, 0xd4ef3085);
		HH(b, c, d, a, x[6], s34, 0x4881d05);
		HH(a, b, c, d, x[9], s31, 0xd9d4d039);
		HH(d, a, b, c, x[12], s32, 0xe6db99e5);
		HH(c, d, a, b, x[15], s33, 0x1fa27cf8);
		HH(b, c, d, a, x[2], s34, 0xc4ac5665);

		/* Round 4 */
		II(a, b, c, d, x[0], s41, 0xf4292244);
		II(d, a, b, c, x[7], s42, 0x432aff97);
		II(c, d, a, b, x[14], s43, 0xab9423a7);
		II(b, c, d, a, x[5], s44, 0xfc93a039);
		II(a, b, c, d, x[12], s41, 0x655b59c3);
		II(d, a, b, c, x[3], s42, 0x8f0ccc92);
		II(c, d, a, b, x[10], s43, 0xffeff47d);
		II(b, c, d, a, x[1], s44, 0x85845dd1);
		II(a, b, c, d, x[8], s41, 0x6fa87e4f);
		II(d, a, b, c, x[15], s42, 0xfe2ce6e0);
		II(c, d, a, b, x[6], s43, 0xa3014314);
		II(b, c, d, a, x[13], s44, 0x4e0811a1);
		II(a, b, c, d, x[4], s41, 0xf7537e82);
		II(d, a, b, c, x[11], s42, 0xbd3af235);
		II(c, d, a, b, x[2], s43, 0x2ad7d2bb);
		II(b, c, d, a, x[9], s44, 0xeb86d391);

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
	}

	// 下面的处理，在理解上较为复杂
	for (int i = 0; i < 4; i++)
	{
		uint32_t value = state[i];
		state[i] = ((value & 0xff) << 24) |		 // 将最低字节移到最高位
				   ((value & 0xff00) << 8) |	 // 将次低字节左移
				   ((value & 0xff0000) >> 8) |	 // 将次高字节右移
				   ((value & 0xff000000) >> 24); // 将最高字节移到最低位
	}

	// 输出最终的hash结果
	// for (int i1 = 0; i1 < 4; i1 += 1)
	// {
	// 	cout << std::setw(8) << std::setfill('0') << hex << state[i1];
	// }
	// cout << endl;

	// 释放动态分配的内存
	// 实现SIMD并行算法的时候，也请记得及时回收内存！
	delete[] paddedMessage;
	delete[] messageLength;
}

void PrepareMessage(const string& input, Byte* output, int* output_length) {
    // 复制原始消息
    size_t input_length = input.length();
    memcpy(output, input.c_str(), input_length);
    
    // 计算填充
    size_t bitLength = input_length * 8;
    size_t padding_bits = bitLength % 512;
    size_t padding_bytes;
    
    if (padding_bits < 448) {
        padding_bytes = (448 - padding_bits) / 8;
    } else {
        padding_bytes = (512 + 448 - padding_bits) / 8;
    }
    
    // 添加填充
    output[input_length] = 0x80;
    
    // 使用预定义的zero_buffer进行快速填充
    if (padding_bytes - 1 <= MAX_BUFFER_SIZE) {
        memcpy(output + input_length + 1, zero_buffer, padding_bytes - 1);
    } else {
        memset(output + input_length + 1, 0, padding_bytes - 1);
    }
    
    // 添加长度字段
    for (int i = 0; i < 8; ++i) {
        output[input_length + padding_bytes + i] = ((uint64_t)bitLength >> (i * 8)) & 0xFF;
    }
    
    *output_length = input_length + padding_bytes + 8;
}

void MD5Hash_SIMD(const string inputs[4], bit32 states[4][4]) {
    // 初始状态值
    bit32x4_t a0 = vdupq_n_u32(0x67452301);
    bit32x4_t b0 = vdupq_n_u32(0xefcdab89);
    bit32x4_t c0 = vdupq_n_u32(0x98badcfe);
    bit32x4_t d0 = vdupq_n_u32(0x10325476);
    
    // 为每个字符串准备填充后的数据
    Byte* paddedMessages[4];
    int messageLengths[4];
    
	// 找出最长的输入
    size_t max_length = std::max({inputs[0].length(), inputs[1].length(), 
		inputs[2].length(), inputs[3].length()});
    
    // 计算所需的最大缓冲区大小
    size_t padded_size = ((max_length + 64 + 8 + 63) / 64) * 64; // 确保64字节对齐
    
    // 确保缓冲区足够大
    if (!reusable_buffers || total_buffer_capacity < 4 * padded_size) {
        delete[] reusable_buffers; // 安全删除之前的缓冲区
        reusable_buffers = new Byte[4 * padded_size];
        total_buffer_capacity = 4 * padded_size;
    }
    
    for (int i = 0; i < 4; i++) {
        paddedMessages[i] = reusable_buffers + i * padded_size;
        PrepareMessage(inputs[i], paddedMessages[i], &messageLengths[i]);
    }
    
    // 确认所有消息块的数量相同
	int n_blocks = messageLengths[0] / 64;
	assert(messageLengths[1] / 64 == n_blocks);
	assert(messageLengths[2] / 64 == n_blocks);
	assert(messageLengths[3] / 64 == n_blocks);

    
    // 处理每个块
    for (size_t block = 0; block < n_blocks; block++) {
         // 使用静态预分配的数组替代栈上数组
		 bit32x4_t* M = M_static;

		 // 预计算块基址偏移，减少每次计算
		const size_t block_base = block * 64;

		

        // 如果不是最后一个块，预加载下一个块的前几个字
    if (block < n_blocks - 1) {
        // 仅预加载下一个块的开始部分
        __builtin_prefetch(paddedMessages[0] + block_base + 64, 0, 3);
        __builtin_prefetch(paddedMessages[1] + block_base + 64, 0, 3);
        __builtin_prefetch(paddedMessages[2] + block_base + 64, 0, 3);
        __builtin_prefetch(paddedMessages[3] + block_base + 64, 0, 3);
    }

    for (int j = 0; j < 16; j++) {
        // 一次性计算偏移量
        const size_t offset = block_base + j * 4;
        
        // 每4个字进行一次预加载，减少预加载指令的数量
        if (j % 4 == 0 && j < 12) {
            // 为当前块的后续数据预加载
            __builtin_prefetch(paddedMessages[0] + offset + 16, 0, 2);
            __builtin_prefetch(paddedMessages[1] + offset + 16, 0, 2);
            __builtin_prefetch(paddedMessages[2] + offset + 16, 0, 2);
            __builtin_prefetch(paddedMessages[3] + offset + 16, 0, 2);
        }
        
        uint32_t values[4] __attribute__((aligned(16)));
    	values[0] = *(uint32_t*)(paddedMessages[0] + offset);
    	values[1] = *(uint32_t*)(paddedMessages[1] + offset);
    	values[2] = *(uint32_t*)(paddedMessages[2] + offset);
    	values[3] = *(uint32_t*)(paddedMessages[3] + offset);
        
        M[j] = vld1q_u32(values);
    }
        
        // 保存当前轮的哈希值
        bit32x4_t A = a0;
        bit32x4_t B = b0;
        bit32x4_t C = c0;
        bit32x4_t D = d0;
        
        // MD5算法的四轮操作(完全按照原始算法)
        
        // 第1轮操作
        FF_SIMD(A, B, C, D, M[0], s11, 0xd76aa478);
        FF_SIMD(D, A, B, C, M[1], s12, 0xe8c7b756);
        FF_SIMD(C, D, A, B, M[2], s13, 0x242070db);
        FF_SIMD(B, C, D, A, M[3], s14, 0xc1bdceee);
        FF_SIMD(A, B, C, D, M[4], s11, 0xf57c0faf);
        FF_SIMD(D, A, B, C, M[5], s12, 0x4787c62a);
        FF_SIMD(C, D, A, B, M[6], s13, 0xa8304613);
        FF_SIMD(B, C, D, A, M[7], s14, 0xfd469501);
        FF_SIMD(A, B, C, D, M[8], s11, 0x698098d8);
        FF_SIMD(D, A, B, C, M[9], s12, 0x8b44f7af);
        FF_SIMD(C, D, A, B, M[10], s13, 0xffff5bb1);
        FF_SIMD(B, C, D, A, M[11], s14, 0x895cd7be);
        FF_SIMD(A, B, C, D, M[12], s11, 0x6b901122);
        FF_SIMD(D, A, B, C, M[13], s12, 0xfd987193);
        FF_SIMD(C, D, A, B, M[14], s13, 0xa679438e);
        FF_SIMD(B, C, D, A, M[15], s14, 0x49b40821);
        
        // 第2轮操作
        GG_SIMD(A, B, C, D, M[1], s21, 0xf61e2562);
        GG_SIMD(D, A, B, C, M[6], s22, 0xc040b340);
        GG_SIMD(C, D, A, B, M[11], s23, 0x265e5a51);
        GG_SIMD(B, C, D, A, M[0], s24, 0xe9b6c7aa);
        GG_SIMD(A, B, C, D, M[5], s21, 0xd62f105d);
        GG_SIMD(D, A, B, C, M[10], s22, 0x02441453);  
        GG_SIMD(C, D, A, B, M[15], s23, 0xd8a1e681);
        GG_SIMD(B, C, D, A, M[4], s24, 0xe7d3fbc8);
        GG_SIMD(A, B, C, D, M[9], s21, 0x21e1cde6);
        GG_SIMD(D, A, B, C, M[14], s22, 0xc33707d6);
        GG_SIMD(C, D, A, B, M[3], s23, 0xf4d50d87);
        GG_SIMD(B, C, D, A, M[8], s24, 0x455a14ed);
        GG_SIMD(A, B, C, D, M[13], s21, 0xa9e3e905);
        GG_SIMD(D, A, B, C, M[2], s22, 0xfcefa3f8);
        GG_SIMD(C, D, A, B, M[7], s23, 0x676f02d9);
        GG_SIMD(B, C, D, A, M[12], s24, 0x8d2a4c8a);
        
        // 第3轮操作
        HH_SIMD(A, B, C, D, M[5], s31, 0xfffa3942);
        HH_SIMD(D, A, B, C, M[8], s32, 0x8771f681);
        HH_SIMD(C, D, A, B, M[11], s33, 0x6d9d6122);
        HH_SIMD(B, C, D, A, M[14], s34, 0xfde5380c);
        HH_SIMD(A, B, C, D, M[1], s31, 0xa4beea44);
        HH_SIMD(D, A, B, C, M[4], s32, 0x4bdecfa9);
        HH_SIMD(C, D, A, B, M[7], s33, 0xf6bb4b60);
        HH_SIMD(B, C, D, A, M[10], s34, 0xbebfbc70);
        HH_SIMD(A, B, C, D, M[13], s31, 0x289b7ec6);
        HH_SIMD(D, A, B, C, M[0], s32, 0xeaa127fa);
        HH_SIMD(C, D, A, B, M[3], s33, 0xd4ef3085);
        HH_SIMD(B, C, D, A, M[6], s34, 0x04881d05);  
        HH_SIMD(A, B, C, D, M[9], s31, 0xd9d4d039);
        HH_SIMD(D, A, B, C, M[12], s32, 0xe6db99e5);
        HH_SIMD(C, D, A, B, M[15], s33, 0x1fa27cf8);
        HH_SIMD(B, C, D, A, M[2], s34, 0xc4ac5665);
        
        // 第4轮操作
        II_SIMD(A, B, C, D, M[0], s41, 0xf4292244);
        II_SIMD(D, A, B, C, M[7], s42, 0x432aff97);
        II_SIMD(C, D, A, B, M[14], s43, 0xab9423a7);
        II_SIMD(B, C, D, A, M[5], s44, 0xfc93a039);
        II_SIMD(A, B, C, D, M[12], s41, 0x655b59c3);
        II_SIMD(D, A, B, C, M[3], s42, 0x8f0ccc92);
        II_SIMD(C, D, A, B, M[10], s43, 0xffeff47d);
        II_SIMD(B, C, D, A, M[1], s44, 0x85845dd1);
        II_SIMD(A, B, C, D, M[8], s41, 0x6fa87e4f);
        II_SIMD(D, A, B, C, M[15], s42, 0xfe2ce6e0);
        II_SIMD(C, D, A, B, M[6], s43, 0xa3014314);
        II_SIMD(B, C, D, A, M[13], s44, 0x4e0811a1);
        II_SIMD(A, B, C, D, M[4], s41, 0xf7537e82);
        II_SIMD(D, A, B, C, M[11], s42, 0xbd3af235);
        II_SIMD(C, D, A, B, M[2], s43, 0x2ad7d2bb);
        II_SIMD(B, C, D, A, M[9], s44, 0xeb86d391);
        
        // 更新状态
        a0 = vaddq_u32(a0, A);
        b0 = vaddq_u32(b0, B);
        c0 = vaddq_u32(c0, C);
        d0 = vaddq_u32(d0, D);
    }
    
    // 提取最终哈希值
    bit32 a_values[4], b_values[4], c_values[4], d_values[4];
    vst1q_u32(a_values, a0);
    vst1q_u32(b_values, b0);
    vst1q_u32(c_values, c0);
    vst1q_u32(d_values, d0);

	// 字节序翻转
	for (int i = 0; i < 4; i++) {
		 // 使用单条指令进行字节翻转更高效
		 states[i][0] = __builtin_bswap32(a_values[i]);
		 states[i][1] = __builtin_bswap32(b_values[i]);
		 states[i][2] = __builtin_bswap32(c_values[i]);
		 states[i][3] = __builtin_bswap32(d_values[i]);
	}
	
}

void MD5Hash_SIMD8(const string inputs[8], bit32 states[8][4]) {
    // 顺序处理8个输入，避免线程创建开销
    MD5Hash_SIMD(&inputs[0], &states[0]);
    MD5Hash_SIMD(&inputs[4], &states[4]);
}


void CleanupMD5Resources() {
    delete[] reusable_buffers;
    reusable_buffers = nullptr;
    total_buffer_capacity = 0;
}
