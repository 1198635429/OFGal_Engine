//这里放置所有在通信中用到的结构体定义，你们按照下面的格式写你们自己的结构体定义
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <math.h>
#include <algorithm>
#include <windows.h>
#include <cuda_runtime.h>

/*
=================================================
文件系统结构体定义
=================================================
*/
struct FolderStructure {
	std::string SelfName;
	std::vector<std::string> Files;
	std::map<std::string, std::unique_ptr<FolderStructure>> SubFolders;
};

struct ProjectStructure {
	std::string RootDirectory;
	FolderStructure Self;
};

struct BMP_Pixel {
	unsigned char red = 0;
	unsigned char green = 0;
	unsigned char blue = 0;
	unsigned char alpha = 0;
};

struct BMP_Data {
	int width;
	int height;
	std::vector<BMP_Pixel> pixels;
};
// 坐标与尺寸辅助结构
struct Location3D {
	float x = 0.0f;
	float y = 0.0f;
	int z = 0;
};

struct Location2D {
	float x = 0.0f;
	float y = 0.0f;
};

struct Location2DInt {
	int x = 0;
	int y = 0;
};

struct RotationEuler {
	float r = 0.0f;
};

struct Scale2D {
	float x = 1.0f;
	float y = 1.0f;
};

struct Size2D {
	float x = 0.0f;   // <=0 表示使用原始图片大小
	float y = 0.0f;
};

struct Size2DInt {
	int x = 0;         // <=0 表示使用默认大小
	int y = 0;
};

// 组件结构
struct TransformComponent {
	Location3D Location;
	RotationEuler Rotation;
	Scale2D Scale;
};

struct PictureComponent {
	std::string Path;
	Location3D Location;
	RotationEuler Rotation;
	Size2D Size;
};

struct TextblockComponent {
	struct TextInfo {
		std::string component;
		int Font_size = 0;      // <=0 表示默认大小
		bool ANSI_Print = false;
	};
	Location2DInt Location;
	Size2DInt Size;
	TextInfo Text;
	Scale2D Scale;
};

struct TriggerAreaComponent {
	Location2D Location;
	Size2D Size;                // <=0 视为0.0（文档要求）
};

struct BlueprintComponent {
	std::string Path;
};

#ifndef __CUDACC__  // 以下内容仅在主机编译时可见
struct ObjectData {
	std::string name;
	ObjectData* parent;		//如果是以场景为父对象，则为 nullptr
	std::map<std::string, ObjectData*> objects;
	std::optional<TransformComponent> Transform;
	std::optional<PictureComponent> Picture;
	std::optional<TextblockComponent> Textblock;
	std::optional<TriggerAreaComponent> TriggerArea;
	std::optional<BlueprintComponent> Blueprint;
};

// 场景结构
struct LevelData {
	std::string name;                                    // 场景名（JSON顶层键）
	std::map<std::string, ObjectData*> objects; // 对象名 -> 对象数据
};
#endif

/*
=================================================
蓝图系统结构体定义
=================================================
*/
#ifndef __CUDACC__  // 以下内容仅在主机编译时可见
// 引脚定义
struct Pin {
	std::string name;
	std::string io;          // "I" 或 "O"
	std::string type;
	std::optional<std::string> literal;   // 字面值，可能不存在
};

// 节点定义
struct Node {
	int id;
	std::string type;
	std::vector<Pin> pins;
	std::map<std::string, std::string> properties;   // 键值对属性
};

// 变量定义
struct Variable {
	std::string name;
	std::string type;
	std::string value;       // 存储字面值（字符串形式）
};

// 参数定义（输入/输出参数）
struct Parameter {
	std::string name;
	std::string type;
	std::string defaultValue;
};

// 事件定义
struct Event {
	std::string event_name;   // 对应 JSON 中的 "Event_Name"
	int id;
};

// 连接定义
struct Link {
	int sourceNode;
	std::string sourcePin;
	int targetNode;
	std::string targetPin;
};

// 蓝图顶层结构
struct BlueprintData {
	std::string name;
	int id;
	std::vector<Node> nodes;
	std::vector<Variable> variables;
	std::vector<Parameter> inParameters;
	std::vector<Parameter> outParameters;
	std::vector<Event> events;
	std::vector<Link> links;
};
#endif
/*
=================================================
渲染系统结构体定义
=================================================
*/
struct StdPixel {
	unsigned char red = 0;
	unsigned char green = 0;
	unsigned char blue = 0;
};
struct Frame {
	int width;
	int height;
	std::vector<StdPixel> pixels;
};
struct Vector3D {		//一般用作列向量
	float x = 0.0f, y = 0.0f, z = 1.0f;
};
struct Matrix3D {
	float m[3][3] = { {1,0,0},{0,1,0} ,{0,0,1} };

	float(&operator[](int i))[3] {return m[i];}
	const float(&operator[](int i) const)[3] {return m[i];}

	Matrix3D operator*(const Matrix3D& other) const {		// 矩阵乘法
		Matrix3D result;
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j){
                result[i][j] = 0;
				for (int k = 0; k < 3; ++k)
					result[i][j] += m[i][k] * other.m[k][j];
		}
		return result;
	}
	Vector3D operator*(const Vector3D& v) const {		// 矩阵右乘向量
		Vector3D result;
		result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
		result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
		result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
		return result;
	}
	Matrix3D operator*(const float scalar)const {		// 矩阵右乘标量
        Matrix3D result;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                result[i][j] = m[i][j] * scalar;
            }
        }
        return result;
	}
	friend Matrix3D operator*(float scalar, const Matrix3D& mat) {		// 矩阵左乘标量
		return mat * scalar;
	}
	// 求逆矩阵（鲁棒版）
	Matrix3D inverse() const {
		// 提取矩阵元素
		float a = m[0][0], b = m[0][1], c = m[0][2];
		float d = m[1][0], e = m[1][1], f = m[1][2];
		float g = m[2][0], h = m[2][1], i = m[2][2];

		// 计算行列式
		float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);

		// 阈值，根据实际需求调整
		const float epsilon = 1e-6f;
		if (std::fabs(det) < epsilon) {
			// 矩阵不可逆，返回单位矩阵（或根据需求抛出异常）
			return Matrix3D(); // 默认构造为单位矩阵
		}

		float invDet = 1.0f / det;
		Matrix3D inv;

		// 伴随矩阵的转置除以行列式
		inv[0][0] = (e * i - f * h) * invDet;
		inv[0][1] = (c * h - b * i) * invDet;
		inv[0][2] = (b * f - c * e) * invDet;
		inv[1][0] = (f * g - d * i) * invDet;
		inv[1][1] = (a * i - c * g) * invDet;
		inv[1][2] = (c * d - a * f) * invDet;
		inv[2][0] = (d * h - e * g) * invDet;
		inv[2][1] = (b * g - a * h) * invDet;
		inv[2][2] = (a * e - b * d) * invDet;

		return inv;
	}
};
struct RenderData {
	Matrix3D trans, inverse_trans;
	Vector3D points[4];		// 存储累积变换后的顶点坐标，左乘逆矩阵获取原坐标
	BMP_Data texture;
	std::vector<int> depth;		// 分级深度，顺序：父 -> 子
};
// 纹理采样方法枚举
enum TextureSamplingMethod {
	SAMPLING_NEAREST = 0,
	SAMPLING_BILINEAR = 1,
	SAMPLING_BICUBIC = 2,
	SAMPLING_ANISOTROPIC = 3
};

/*
=================================================
音频系统结构体定义
=================================================
*/

struct AudioClip {
	std::string path;        // 音频文件的绝对路径
	bool loop;               // 是否循环播放
	float volume;            // 音频的音量
	bool isPaused;           // 音频是否被暂停
	std::shared_ptr<void> audioData;  // 音频数据的智能指针

	// 默认构造函数
	AudioClip()
		: path(""), loop(false), volume(1.0f), isPaused(false) {
	}

	// 构造函数，初始化音频路径和循环标志,
	AudioClip(const std::string& path, bool loop)
		: path(path), loop(loop), volume(1.0f), isPaused(false) {
	}
};

/* 
===================================================
窗口管理系统结构体定义
===================================================
*/
// 单个共享内存块描述
struct SharedMemBlock {
	HANDLE hSharedMem = NULL;   // 文件映射句柄
	char* pView = nullptr;      // 映射视图指针
	DWORD size = 0;             // 块大小（字节）
	bool isReadOnly = false;    // 是否只读（用于从子进程接收数据的块，主进程只读）
};

// 子进程信息
struct ChildProcessInfo {
	HANDLE hProcess = NULL;             // 进程句柄
	HANDLE hThread = NULL;              // 主线程句柄
	DWORD processId = 0;                // 进程ID（便于外部使用）
	std::unordered_map<std::string, HANDLE> events;        // 命名事件句柄（主进程创建）
	std::unordered_map<std::string, SharedMemBlock> sharedMems; // 共享内存块

	// 以下用于双向通信时的反向事件（由子进程创建，主进程打开）
	std::unordered_map<std::string, HANDLE> childEvents;   // 子进程创建的事件句柄
};

// 子进程启动配置
struct ChildProcessConfig {
	std::string processKey;                     // 唯一标识符（如 "ProjectStructureViewer"）
	std::string exePath;                        // 可执行文件路径
	std::string commandLineArgs;                // 额外命令行参数（不含 exe 本身）
	bool createNewConsole = true;               // 是否创建新控制台窗口（默认是）
	bool redirectStdIO = true;                  // 是否重定向标准 IO 到新控制台（仅当 createNewConsole 为 true 时有效）

	// 需要创建的共享内存块配置：块名 -> 大小（字节）
	std::unordered_map<std::string, DWORD> sharedMemBlocks;

	// 需要创建的事件配置：事件名 -> 初始状态（false=未触发）
	std::unordered_map<std::string, bool> eventsToCreate;
};