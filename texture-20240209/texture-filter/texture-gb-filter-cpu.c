#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define IMG_WIDTH	1920
#define IMG_HEIGHT	1080

// イメージ型の定義
struct image_t {
	int	width;			// 画像幅
	int	height;			// 画像高さ
	size_t	bytes;		// 画像データのバイト数
	uint8_t	*buff;		// 画像データバッファ
};

// RGB 画像データ型の定義
struct rgb_t {
	uint8_t	r;			// Red
	uint8_t	g;			// Green
	uint8_t	b;			// Blue
};

// 2D 頂点座標型の定義
struct vertex_t {
	int	x;
	int	y;
};

// 描画用の頂点座標配列：長方形
struct vertex_t vertices[] = {
	{             0,              0 },		// (   0,    0)：左上
	{ IMG_WIDTH - 1, IMG_HEIGHT - 1 },		// (1919, 1079)：右下
};

// 関数のプロトタイプ宣言
struct image_t *create_image(int width, int height);
void read_raw_image_file(const char *file, struct image_t *img);
void write_raw_image_file(const char *file, struct image_t *img);
void draw_box(struct vertex_t *ver, struct image_t *tex, struct image_t *fb);
struct vertex_t vertex_shader(struct vertex_t ver);
struct rgb_t fragment_shader(struct image_t *tex, int x, int y);
struct rgb_t texture2D(struct image_t *tex, int x, int y);


int main(int argc, char *argv[])
{
	struct image_t	*texture, *framebuff;

	// テクスチャバッファとフレームバッファを生成する。
	texture = create_image(IMG_WIDTH, IMG_HEIGHT);
	framebuff = create_image(IMG_WIDTH, IMG_HEIGHT);

	// 指定された RAW 画像ファイルをテクスチャバッファに読み込む。
	read_raw_image_file(argv[1], texture);

	// 長方形を描画する。
	draw_box(vertices, texture, framebuff);

	// 処理結果を RAW 画像ファイルに書き込む。
	write_raw_image_file(argv[2], framebuff);
}

// イメージ型の変数(インスタンス)を生成する関数。
struct image_t *create_image(int width, int height)
{
	struct image_t	*img;

	img = (struct image_t *)malloc(sizeof(struct image_t));
	img->width = width;
	img->height = height;
	img->bytes = (size_t)width * height * 3;		// RGB 画像を想定
	img->buff = (uint8_t *)malloc(img->bytes);
	return img;
}

void read_raw_image_file(const char *file, struct image_t *img)
{
	FILE	*fp;
	size_t	read_size;

	fp = fopen(file, "r");
	assert(fp != NULL);
	read_size = fread(img->buff, 1, img->bytes, fp);
	assert(read_size == img->bytes);
	fclose(fp);
}

void write_raw_image_file(const char *file, struct image_t *img)
{
	FILE	*fp;
	size_t	write_size;

	fp = fopen(file, "w");
	assert(fp != NULL);
	write_size = fwrite(img->buff, 1, img->bytes, fp);
	assert(write_size == img->bytes);
	fclose(fp);
}

// 長方形を描画する関数。
void draw_box(struct vertex_t *ver, struct image_t *tex, struct image_t *fb)
{
	int	i, x, y;
	struct rgb_t	*dest;
	struct vertex_t	gl_position[2];

	// バーテックスシェーダー関数を実行する。
	for (i = 0; i < 2; i++) {
		gl_position[i] = vertex_shader(ver[i]);
	}

	// フラグメントシェーダー関数を実行する。
	dest = (struct rgb_t *)fb->buff;
	for (y = gl_position[0].y; y <= gl_position[1].y; y++) {
		for (x = gl_position[0].x; x <= gl_position[1].x; x++) {
			*dest++ = fragment_shader(tex, x, y);
		}
	}
}

// バーテックスシェーダー関数
struct vertex_t vertex_shader(struct vertex_t ver)
{
	// 受け取った頂点座標を描画用の頂点座標としてそのまま返す。
	return ver;
}

// フラグメントシェーダー関数
struct rgb_t fragment_shader(struct image_t *tex, int x, int y)
{
	int	i, j, xref, yref, sum_r, sum_g, sum_b;
	static int	filter[3][3] = {
		19, 32, 19,					// 0.075*256 = 19.2 → 19
		32, 52, 32,					// 0.124*256 = 31.7 → 32
		19, 32, 19					// 0.204*256 = 52.2 → 52
	};
	struct rgb_t	pix, gl_fragcolor;

	// 3x3 の参照画素に対して畳み込み演算を行う。
	sum_r = sum_g = sum_b = 0;
	for (i = 0; i < 3; i++) {
		yref = y + i - 1;
		for (j = 0; j < 3; j++) {
			xref = x + j -1;

			// テクスチャを参照して参照座標の画素値を取り出す。
			pix = texture2D(tex, xref, yref);

			// 参照した画素値にフィルタ係数を掛けて積算する。
			sum_r += pix.r * filter[i][j];
			sum_g += pix.g * filter[i][j];
			sum_b += pix.b * filter[i][j];
		}
	}

	// フィルタ係数に 256 倍の下駄を履かせたので、最後に調整する。
	gl_fragcolor.r = sum_r / 256;
	gl_fragcolor.g = sum_g / 256;
	gl_fragcolor.b = sum_b / 256;

	return gl_fragcolor;
}

// テクスチャから１画素分の画素値を取り出す関数
struct rgb_t texture2D(struct image_t *tex, int x, int y)
{
	struct rgb_t *ref;

	// テクスチャ参照座標が有効範囲内となるようにクリッピングする。
	if (x < 0) x = 0;
	if (x >= tex->width) x = tex->width - 1;
	if (y < 0) y = 0;
	if (y >= tex->height) y = tex->height - 1;

	// テクスチャから参照座標位置の画素値を得る。
	ref = (struct rgb_t *)tex->buff + (y * tex->width) + x;
	return *ref;
}

