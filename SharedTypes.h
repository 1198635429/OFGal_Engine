//这里放置所有在通信中用到的结构体定义，你们按照下面的格式写你们自己的结构体定义
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <optional>

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
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
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

struct Rotation {
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
	Rotation Rotation;
	Scale2D Scale;
};

struct PictureComponent {
	std::string Path;
	Location3D Location;
	Rotation Rotation;
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
	std::string parent;
	std::vector<std::string> children;
	std::optional<TransformComponent> Transform;
	std::optional<PictureComponent> Picture;
	std::optional<TextblockComponent> Textblock;
	std::optional<TriggerAreaComponent> TriggerArea;
	std::optional<BlueprintComponent> Blueprint;
};

// 场景结构
struct LevelData {
	std::string name;                                    // 场景名（JSON顶层键）
	std::unordered_map<std::string, ObjectData> objects; // 对象名 -> 对象数据
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
	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
};
struct Frame {
	int width;
	int height;
	std::vector<StdPixel> pixels;
};