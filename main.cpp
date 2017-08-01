//
// OBJ形式を扱う
//

// TIPS:M_PIとかを使えるようにする
#define _USE_MATH_DEFINES

#include <cmath>
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <iostream>
#include <vector>

// 画像を扱う
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLFWのヘッダ内で他のライブラリを取り込む
#define GLFW_INCLUDE_GLEXT
#define GLFW_INCLUDE_GLU

#if defined(_MSC_VER)
// Windows:GLEWをスタティックライブラリ形式で利用
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>


#if defined(_MSC_VER)
// Windows:外部ライブラリのリンク指定
#if defined (_DEBUG)
#pragma comment(lib, "glfw3d.lib")
#pragma comment(lib, "glew32sd.lib")
#else
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "glew32s.lib")
#endif
#pragma comment(lib, "OPENGL32.lib")
#pragma comment(lib, "GLU32.lib")
#endif


// 読み込んだOBJ形式の情報
// NOTICE:OpenGLにそのまま渡してglDrawArraysで描画する
//        構造になっている
struct Obj {
  std::vector<GLfloat> vtx;
  std::vector<GLfloat> normal;
  std::vector<GLfloat> uv;
};


// OBJ形式を読み込む
Obj loadObj(const std::string& path) {
  std::ifstream stream(path);
  assert(stream);

  std::vector<GLfloat> normal;
  std::vector<GLfloat> uv;
  std::vector<GLfloat> vtx;

  Obj obj;

  while (!stream.eof()) {
    // セクションを探す
    std::string s;
    std::getline(stream, s);
    if (s == "# normals") {
      std::cout << "normals" << std::endl;
      while (!stream.eof()) {
        // 一行ずつ読み込んで変換
        std::string s;
        std::getline(stream, s);

        // TIPS:文字列ストリームを使い
        //      文字列→数値のプログラム負荷を下げている
        std::stringstream ss;
        ss << s;

        std::string key;
        ss >> key;
        if (key != "vn") break;

        float x, y, z;
        ss >> x >> y >> z;
        normal.push_back(x);
        normal.push_back(y);
        normal.push_back(z);
      }
      std::cout << normal.size() / 3 << std::endl;
    }
    else if (s == "# texcoords") {
      std::cout << "texcoords" << std::endl;
      while (!stream.eof()) {
        std::string s;
        std::getline(stream, s);

        std::stringstream ss;
        ss << s;

        std::string key;
        ss >> key;
        if (key != "vt") break;

        float x, y;
        ss >> x >> y;
        uv.push_back(x);
        uv.push_back(y);
      }
      std::cout << uv.size() / 2 << std::endl;
    }
    else if (s == "# verts") {
      std::cout << "verts" << std::endl;
      while (!stream.eof()) {
        std::string s;
        std::getline(stream, s);

        std::stringstream ss;
        ss << s;

        std::string key;
        ss >> key;
        if (key != "v") break;

        float x, y, z;
        ss >> x >> y >> z;
        vtx.push_back(x);
        vtx.push_back(y);
        vtx.push_back(z);
      }
      std::cout << vtx.size() / 3 << std::endl;
    }
    else if (s == "# faces") {
      std::cout << "faces" << std::endl;
      while (!stream.eof()) {
        std::string s;
        std::getline(stream, s);

        std::stringstream ss;
        ss << s;

        std::string key;
        ss >> key;
        if (key!= "f") break;

        // FIXME:三角形固定
        int vi[3];
        int ti[3];
        int ni[3];
        char separate;
        
        ss >> vi[0] >> separate >> ti[0] >> separate >> ni[0];
        ss >> vi[1] >> separate >> ti[1] >> separate >> ni[1];
        ss >> vi[2] >> separate >> ti[2] >> separate >> ni[2];

        // ３頂点の座標・UV・法線の値をコンテナに格納
        for (int i = 0; i < 3; ++i) {
          obj.vtx.push_back(vtx[(vi[i] - 1) * 3 + 0]);
          obj.vtx.push_back(vtx[(vi[i] - 1) * 3 + 1]);
          obj.vtx.push_back(vtx[(vi[i] - 1) * 3 + 2]);

          obj.uv.push_back(uv[(ti[i] - 1) * 2 + 0]);
          obj.uv.push_back(uv[(ti[i] - 1) * 2 + 1]);
          
          obj.normal.push_back(normal[(ni[i] - 1) * 3 + 0]);
          obj.normal.push_back(normal[(ni[i] - 1) * 3 + 1]);
          obj.normal.push_back(normal[(ni[i] - 1) * 3 + 2]);
        }
      }
    }
  }

  return obj;
}


// 画像を読み込む
GLuint createTexture(std::string path) {
  GLuint id;
  glGenTextures(1, &id);

  glBindTexture(GL_TEXTURE_2D, id);

  int width;
  int height;
  int comp;
  unsigned char *data = stbi_load(path.c_str(), &width, &height, &comp, 0);

  GLint type = (comp == 3)
               ? GL_RGB
               : GL_RGBA;

  glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  return id;
}


int main() {
  // 初期化
  if (!glfwInit()) return -1;

  // Window生成
  GLFWwindow* window = glfwCreateWindow(800,
                                        600,
                                        "OBJ loader",
                                        nullptr, nullptr);

  if (!window) {
    // 生成失敗
    glfwTerminate();
    return -1;
  }

  // OpenGLをWindowで使えるようにする
  glfwMakeContextCurrent(window);
  // Window画面の更新タイミングをPCディスプレイに同期する
  glfwSwapInterval(1);

#if defined (_MSC_VER)
  // GLEWを初期化(Windows)
  if (glewInit() != GLEW_OK) {
    // パソコンがOpenGL拡張に対応していなければ終了
    glfwTerminate();
    return -1;
  }
#endif

  // これ以降OpenGLの命令が使える
  GLuint texture_id = createTexture("chr_rain.png");
  glEnable(GL_TEXTURE_2D);
  
  Obj obj = loadObj("chr_rain.obj");
  glVertexPointer(3, GL_FLOAT, 0, &obj.vtx[0]);
  glNormalPointer(GL_FLOAT, 0, &obj.normal[0]);
  glTexCoordPointer(2, GL_FLOAT, 0, &obj.uv[0]);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // 透視変換行列を設定
  glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
  gluPerspective(35.0, 640 / 480.0, 0.2, 200.0);

  // 操作対象の行列をモデリングビュー行列に切り替えておく
  glMatrixMode(GL_MODELVIEW);

  float r = 0.0;
  
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // 並行光源の設定
  GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

  GLfloat ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

  GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

  while (!glfwWindowShouldClose(window)) {
    // 描画バッファの初期化
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 単位行列を読み込む
    glLoadIdentity();

    // ライトの設定
    GLfloat position[] = { 0.0f, 0.0f, 4.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glTranslatef(0, 0, -60.0);
    glRotatef(r, 0, 1, 0);
    glRotatef(r, 1, 0, 0);
    glTranslatef(0, -8, 0.0);
    r += 1.0;

    // 描画
    glDrawArrays(GL_TRIANGLES, 0, obj.vtx.size() / 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // テクスチャの拘束を解除して破棄
  glBindTexture(GL_TEXTURE_2D, 0);
  glDeleteTextures(1, &texture_id);

  glfwTerminate();
}
