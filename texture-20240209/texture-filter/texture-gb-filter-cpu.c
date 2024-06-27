#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define IMG_WIDTH	1920
#define IMG_HEIGHT	1080

// �C���[�W�^�̒�`
struct image_t {
	int	width;			// �摜��
	int	height;			// �摜����
	size_t	bytes;		// �摜�f�[�^�̃o�C�g��
	uint8_t	*buff;		// �摜�f�[�^�o�b�t�@
};

// RGB �摜�f�[�^�^�̒�`
struct rgb_t {
	uint8_t	r;			// Red
	uint8_t	g;			// Green
	uint8_t	b;			// Blue
};

// 2D ���_���W�^�̒�`
struct vertex_t {
	int	x;
	int	y;
};

// �`��p�̒��_���W�z��F�����`
struct vertex_t vertices[] = {
	{             0,              0 },		// (   0,    0)�F����
	{ IMG_WIDTH - 1, IMG_HEIGHT - 1 },		// (1919, 1079)�F�E��
};

// �֐��̃v���g�^�C�v�錾
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

	// �e�N�X�`���o�b�t�@�ƃt���[���o�b�t�@�𐶐�����B
	texture = create_image(IMG_WIDTH, IMG_HEIGHT);
	framebuff = create_image(IMG_WIDTH, IMG_HEIGHT);

	// �w�肳�ꂽ RAW �摜�t�@�C�����e�N�X�`���o�b�t�@�ɓǂݍ��ށB
	read_raw_image_file(argv[1], texture);

	// �����`��`�悷��B
	draw_box(vertices, texture, framebuff);

	// �������ʂ� RAW �摜�t�@�C���ɏ������ށB
	write_raw_image_file(argv[2], framebuff);
}

// �C���[�W�^�̕ϐ�(�C���X�^���X)�𐶐�����֐��B
struct image_t *create_image(int width, int height)
{
	struct image_t	*img;

	img = (struct image_t *)malloc(sizeof(struct image_t));
	img->width = width;
	img->height = height;
	img->bytes = (size_t)width * height * 3;		// RGB �摜��z��
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

// �����`��`�悷��֐��B
void draw_box(struct vertex_t *ver, struct image_t *tex, struct image_t *fb)
{
	int	i, x, y;
	struct rgb_t	*dest;
	struct vertex_t	gl_position[2];

	// �o�[�e�b�N�X�V�F�[�_�[�֐������s����B
	for (i = 0; i < 2; i++) {
		gl_position[i] = vertex_shader(ver[i]);
	}

	// �t���O�����g�V�F�[�_�[�֐������s����B
	dest = (struct rgb_t *)fb->buff;
	for (y = gl_position[0].y; y <= gl_position[1].y; y++) {
		for (x = gl_position[0].x; x <= gl_position[1].x; x++) {
			*dest++ = fragment_shader(tex, x, y);
		}
	}
}

// �o�[�e�b�N�X�V�F�[�_�[�֐�
struct vertex_t vertex_shader(struct vertex_t ver)
{
	// �󂯎�������_���W��`��p�̒��_���W�Ƃ��Ă��̂܂ܕԂ��B
	return ver;
}

// �t���O�����g�V�F�[�_�[�֐�
struct rgb_t fragment_shader(struct image_t *tex, int x, int y)
{
	int	i, j, xref, yref, sum_r, sum_g, sum_b;
	static int	filter[3][3] = {
		19, 32, 19,					// 0.075*256 = 19.2 �� 19
		32, 52, 32,					// 0.124*256 = 31.7 �� 32
		19, 32, 19					// 0.204*256 = 52.2 �� 52
	};
	struct rgb_t	pix, gl_fragcolor;

	// 3x3 �̎Q�Ɖ�f�ɑ΂��ď�ݍ��݉��Z���s���B
	sum_r = sum_g = sum_b = 0;
	for (i = 0; i < 3; i++) {
		yref = y + i - 1;
		for (j = 0; j < 3; j++) {
			xref = x + j -1;

			// �e�N�X�`�����Q�Ƃ��ĎQ�ƍ��W�̉�f�l�����o���B
			pix = texture2D(tex, xref, yref);

			// �Q�Ƃ�����f�l�Ƀt�B���^�W�����|���ĐώZ����B
			sum_r += pix.r * filter[i][j];
			sum_g += pix.g * filter[i][j];
			sum_b += pix.b * filter[i][j];
		}
	}

	// �t�B���^�W���� 256 �{�̉��ʂ𗚂������̂ŁA�Ō�ɒ�������B
	gl_fragcolor.r = sum_r / 256;
	gl_fragcolor.g = sum_g / 256;
	gl_fragcolor.b = sum_b / 256;

	return gl_fragcolor;
}

// �e�N�X�`������P��f���̉�f�l�����o���֐�
struct rgb_t texture2D(struct image_t *tex, int x, int y)
{
	struct rgb_t *ref;

	// �e�N�X�`���Q�ƍ��W���L���͈͓��ƂȂ�悤�ɃN���b�s���O����B
	if (x < 0) x = 0;
	if (x >= tex->width) x = tex->width - 1;
	if (y < 0) y = 0;
	if (y >= tex->height) y = tex->height - 1;

	// �e�N�X�`������Q�ƍ��W�ʒu�̉�f�l�𓾂�B
	ref = (struct rgb_t *)tex->buff + (y * tex->width) + x;
	return *ref;
}

