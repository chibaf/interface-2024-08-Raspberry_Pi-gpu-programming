#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>


// 定数データの定義 ---------------------------------------------------------

// EGL フレームバッファ設定を選択するための属性情報リスト
static const EGLint	conf_attrib[] = {
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE
};

// EGL レンダリングコンテキスト生成時に指定する属性情報リスト
static const EGLint	ctx_attrib[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};


int main(int argc, char *argv[])
{
	int	fd;
	const char	*dev = "/dev/dri/card0";
	struct gbm_device	*gbm;
	EGLint	num_config;
	EGLDisplay	display;
	EGLConfig	config;
	EGLContext	context;
	GLint	num, dims[2];

	// コマンドライン引数を評価する。
	if (argc > 1) dev = argv[1];

	// DRM デバイスをオープンする。
	fd = open(dev, O_RDWR);

	// EGL ライブラリを利用して OpenGL ES を初期化する。
	gbm = gbm_create_device(fd);
	display = eglGetDisplay(gbm);
	eglInitialize(display, NULL, NULL);
	eglChooseConfig(display, conf_attrib, &config, 1, &num_config);
	eglBindAPI(EGL_OPENGL_ES_API);
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attrib);
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);

	// OpenGL デバイスのバージョン情報等を表示する。
	printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
//	printf("OpenGL Extensions: %s\n", glGetString(GL_EXTENSIONS));

	// 取り扱い可能な各種リソースの上限を表示する。
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &num);
	printf("Max texture size: %dx%d\n", num, num);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &num);
	printf("Max texture image units: %d\n", num);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &num);
	printf("Max renderbuffer size: %dx%d\n", num, num);
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, dims);
	printf("Max viewport dims: %dx%d\n", dims[0], dims[1]);
}

