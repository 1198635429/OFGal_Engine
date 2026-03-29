# SRP渲染流程（逆向映射）
## 数据准备
获取当前场景中所有的对象数据引用
根据配置信息来决定是否启用超采样、超采样方案
### 超采样
#### 方法1：SSAA全屏超采样
#### 方法2：MSAA多重采样抗锯齿
## 1.累积变换
### TRS矩阵及其逆矩阵
在二维齐次坐标系（3×3矩阵）下，按照 **TRS 顺序**（平移 × 旋转 × 缩放）合并后的矩阵为：
$$
\mathbf{M} = \begin{bmatrix}
1 & 0 & t_x \\
0 & 1 & t_y \\
0 & 0 & 1
\end{bmatrix}
\cdot
\begin{bmatrix}
\cos\theta & \sin\theta & 0 \\
-\sin\theta & \cos\theta & 0 \\
0 & 0 & 1
\end{bmatrix}
\cdot
\begin{bmatrix}
s_x & 0 & 0 \\
0 & s_y & 0 \\
0 & 0 & 1
\end{bmatrix}
$$
计算结果为：
$$
\mathbf{M} = \begin{bmatrix}
s_x \cos\theta & s_y \sin\theta & t_x \\
-s_x \sin\theta & s_y \cos\theta & t_y \\
0 & 0 & 1
\end{bmatrix}
$$

其中：
- \((t_x, t_y)\) 为平移量（世界坐标下）
- \(\theta\) 为旋转角（逆时针为正，适用于坐标系：X向下、Y向右）
- \((s_x, s_y)\) 为缩放因子（沿本地坐标轴）
### 专用数据结构
用场景中所有有非空图片组件的对象初始化如下结构体
```c++
struct RenderData {
	Matrix3D trans, inverse_trans;
	Vector3D points[4];		// 存储累积变换后的顶点坐标
	BMP_Data texture;
	std::vector<int> depth;
};
```
其中：
- trans表示累积变换矩阵，纹理坐标可以通过它映射到画布坐标
- inverse_trans表示累积变换逆矩阵
- points表示变换后的最终顶点集合，使用三维列向量表示
- texture表示源纹理，不推荐拷贝
- depth表示分级深度，如3-1-2
## 2.快速剔除
### 方法1：AABB粗略估计
### 方法2：分离轴定理
## 3.光栅化
## 4.采样
### 方法1：最近邻采样
### 方法2：双线性插值
### 方法3：双三次插值
### 方法4：各向异性过滤
## 5.循环绘制
## 6.后处理
### Bloom泛光
### 抗锯齿
#### FXAA
#### SMAA
### 景深
### 定向模糊
### 颜色分级与调色
### 胶片颗粒与晕影
### 锐化
### 镜头畸变及色差
## 7.输出