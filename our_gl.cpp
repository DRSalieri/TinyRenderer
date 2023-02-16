
#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

// MVP矩阵会跟随相机的变化而变化
Matrix ModelView;
Matrix Projection;
Matrix Viewport;

IShader::~IShader() {}

// 更新V矩阵
void viewport(int x, int y, int w, int h) {
    Viewport = Matrix::identity();
    Viewport[0][3] = x+w/2.f;
    Viewport[1][3] = y+h/2.f;
    Viewport[2][3] = 255.f/2.f;
    Viewport[0][0] = w/2.f;
    Viewport[1][1] = h/2.f;
    Viewport[2][2] = 255.f/2.f;
}

// 更新P矩阵
void projection(float coeff) {
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

// 调整摄像机，计算新的坐标系
//eye - 摄像机坐标
//up - 摄像机up向量
//center - 坐标系原点
void lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye-center).normalize();         // eye指向center的向量为z正方向
    Vec3f x = cross(up,z).normalize();          // up与z的叉积计算x
    Vec3f y = cross(z,x).normalize();           // z与x的叉积计算y
    ModelView = Matrix::identity();
    /**
     *      x0  x1  x2  -c0 
     *      y0  y1  y2  -c1
     *      z0  z1  z2  -c2
     *      0   0   0   1
     **/             
    // 更新M矩阵
    for (int i=0; i<3; i++) {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
}

// 求重心坐标
// 传入坐标是三角形三个点投影到xy平面上，不需要点的深度信息
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    /**
     *  AC[0]   AB[0]   PA[0]
     *  AC[1]   AB[1]   PA[1]
     **/
    for (int i=1; i>=0;i-- ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    /**
     * u[0] = AB[0]*PA[1] - PA[0]*AB[1]
     * u[1] = PA[0]*AC[1] - PA[1]*AC[0]
     * u[2] = AC[0]*AB[1] - AB[0]*AC[1]
     **/
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}
// 画三角：
// requirements:
// It should be (surprise!) simple and fast.
// It should be symmetrical: the picture should not depend on the order of vertices passed to the drawing function.
// If two triangles have two common vertices, there should be no holes between them because of rasterization rounding.
// method:
// Sort vertices of the triangle by their y-coordinates;
// Rasterize simultaneously the left and the right sides of the triangle;
// Draw a horizontal line segment between the left and the right boundary points.

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, float *zbuffer) {
    // 1. 求包围盒
    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::min(bboxmin[j], pts[i][j]/pts[i][3]);
            bboxmax[j] = std::max(bboxmax[j], pts[i][j]/pts[i][3]);
        }
    }
    // 2.遍历包围盒内的每个点，若在质心坐标内，则z-buffer绘图
    Vec2i P;
    TGAColor color;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            // 不需要深度信息，用xy维计算重心坐标
            Vec3f c = barycentric(proj<2>(pts[0]/pts[0][3]), proj<2>(pts[1]/pts[1][3]), proj<2>(pts[2]/pts[2][3]), proj<2>(P));
            float z = pts[0][2]*c.x + pts[1][2]*c.y + pts[2][2]*c.z;        // 深度插值
            float w = pts[0][3]*c.x + pts[1][3]*c.y + pts[2][3]*c.z;        // 齐次坐标插值
            int frag_depth = z/w;// std::max(0, std::min(255, int(z/w+.5)));       // 对depth进行clamp(0,255)
            if (c.x<0 || c.y<0 || c.z<0 || zbuffer[P.x+ P.y * image.get_width()]>frag_depth) continue;   // 如果点不在三角形内、或z-buffer
            bool discard = shader.fragment(c, color);                       // 用shader进行面元着色
            if (!discard) {
                zbuffer[P.x+P.y*image.get_width()] = frag_depth;
                image.set(P.x, P.y, color);
            }
        }
    }
}

// 画线：braseham算法
// 1.消除浮点数
// 2.消除除法
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) { 
    int dx = std::abs(x1-x0);
    int sx = x0<x1 ? 1 : -1;
    int dy = -std::abs(y1-y0);
    int sy = y0<y1 ? 1 : -1;
    int err = dx+dy;  /* error value e_xy */
    while (true){   /* loop */
        image.set(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2*err;
        if (e2 >= dy){ /* e_xy+e_x > 0 */
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx){ /* e_xy+e_y < 0 */
            err += dx;
            y0 += sy;
        }
    }
}