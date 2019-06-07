#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "edify/expr.h"

static const char *DEVICE = "/dev/block/mmcblk0";
extern int check_ext4_compress(char *source, unsigned long long part_size);
extern int writeCompressedExt4Image(char *imgBase, size_t imgSize, int fd);

static int UpdateEXT4(const char *dev, int part, char *buf, size_t size)
{
	char *buffer;
	buffer = buf;
	char mmc_node[128];
	int fd;

	check_ext4_compress(buffer, 0x6400000);

	sprintf(mmc_node, "%sp%d", dev, part);
	fprintf(stderr, "mmc_node = %s \n", mmc_node);
	fd = open(mmc_node, O_RDWR);
	writeCompressedExt4Image(buffer, size, fd);
	close(fd);
	return 0;
}

static int UpdateMMC(const char *dev, off_t offset, char *buf, size_t size)
{
	int ret;
	int fd;
	int i;
	char zero=0;

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: Failed to open %s\n", __func__, dev);
		return -ENODEV;
	}

	ret = lseek(fd, offset, SEEK_SET);
	if (ret != offset) {
		fprintf(stderr, "%s: lseek returned invalid offset(%d/%ld)\n", __func__,
				ret, offset);
		close(fd);
		return -EINVAL;
	}


	ret = write(fd, buf, size);
	if (ret != (int)size) {
		fprintf(stderr, "%s: write returned invalid size(%d/%zu)\n", __func__,
				ret, size);
		close(fd);
		return ret;
	}

	close(fd);
	fprintf(stdout, "Succeed to update of %s, offset %ld, size %zu\n", dev,
			offset, size);
	return 0;
}

static int UpdateBootloader(char *imgBase, size_t imgSize, char *type)
{
	if (!strncmp(type, "mmc", 3))
		return UpdateMMC(DEVICE, 0x200, imgBase, imgSize);

	fprintf(stderr, "%s: Currently only support MMC type\n", __func__);
	return -ENOTSUP;
}

static int UpdateKernel(char *imgBase, size_t imgSize, char *address)
{
	int offset = (int) strtol(address, NULL, 0);

	return UpdateMMC(DEVICE, 0xA00000, imgBase, imgSize);

}

static int UpdateDTB(char *imgBase, size_t imgSize, char *address)
{

	int offset = (int) strtol(address, NULL, 0);

	return UpdateMMC(DEVICE, offset, imgBase, imgSize);

}

static int UpdateRoot(char *imgBase, size_t imgSize, char *part)
{
	int partnum = (int) strtol(part, NULL, 0);

	return UpdateEXT4(DEVICE, partnum, imgBase, imgSize);

}
static Value *WriteKernelFn(const char *name, State *state, int /* argc */,
								Expr *argv[])
{
	int ret;
	Value *img;
	Value *type;

	fprintf(stdout, "name %s\n", __func__, name);
	ret = ReadValueArgs(state, argv, 2, &img, &type);
	if (ret < 0)
		fprintf(stderr, "Failed to ReadValueArgs, ret %d\n", ret);

	if (ret == 0 && (img->type != VAL_BLOB || type->type != VAL_STRING)) {
		FreeValue(img);
		FreeValue(type);
		ret = -EINVAL;
	}

	if (ret == 0) {
		ret = UpdateKernel(img->data, img->size, type->data);
		fprintf(stdout, "UpdateKernel ret %d\n", ret);
	}

	FreeValue(img);
	FreeValue(type);

	return StringValue(strdup(ret ? "": "t"));
}

static Value *WriteDTBFn(const char *name, State *state, int /* argc */,
								Expr *argv[])
{
	int ret;
	Value *img;
	Value *offset;

	fprintf(stdout, "name %s\n", __func__, name);
	ret = ReadValueArgs(state, argv, 2, &img, &offset);
	if (ret < 0)
		fprintf(stderr, "Failed to ReadValueArgs, ret %d\n", ret);

	if (ret == 0 && (img->type != VAL_BLOB || offset->type != VAL_STRING )) {
		FreeValue(img);
		FreeValue(offset);
		ret = -EINVAL;
	}

	if (ret == 0) {
		ret = UpdateDTB(img->data, img->size, offset->data);
		fprintf(stdout, "UpdateDTB ret %d\n", ret);
	}

	FreeValue(img);
	FreeValue(offset);

	return StringValue(strdup(ret ? "": "t"));
}
static Value *WriteRootFn(const char *name, State *state, int /* argc */,
								Expr *argv[])
{
	int ret;
	Value *img;
	Value *type;

	fprintf(stdout, "name %s\n", __func__, name);
	ret = ReadValueArgs(state, argv, 2, &img, &type);
	if (ret < 0)
		fprintf(stderr, "Failed to ReadValueArgs, ret %d\n", ret);

	if (ret == 0 && (img->type != VAL_BLOB || type->type != VAL_STRING)) {
		FreeValue(img);
		FreeValue(type);
		ret = -EINVAL;
	}

	if (ret == 0) {
		ret = UpdateRoot(img->data, img->size, type->data);
		fprintf(stdout, "UpdateRoot ret %d\n", ret);
	}

	FreeValue(img);
	FreeValue(type);

	return StringValue(strdup(ret ? "": "t"));
}


static Value *WriteBootloaderFn(const char *name, State *state, int /* argc */,
								Expr *argv[])
{
	int ret;
	Value *img;
	Value *type;

	fprintf(stdout, "name %s\n", __func__, name);
	ret = ReadValueArgs(state, argv, 2, &img, &type);
	if (ret < 0)
		fprintf(stderr, "Failed to ReadValueArgs, ret %d\n", ret);

	if (ret == 0 && (img->type != VAL_BLOB || type->type != VAL_STRING)) {
		FreeValue(img);
		FreeValue(type);
		ret = -EINVAL;
	}

	if (ret == 0) {
		ret = UpdateBootloader(img->data, img->size, type->data);
		fprintf(stdout, "UpdateBootloader ret %d\n", ret);
	}

	FreeValue(img);
	FreeValue(type);

	return StringValue(strdup(ret ? "": "t"));
}

void Register_librecovery_updater_nexell()
{
	RegisterFunction("nexell.write_bootloader", WriteBootloaderFn);
	RegisterFunction("nexell.write_kernel", WriteKernelFn);
	RegisterFunction("nexell.write_dtb", WriteDTBFn);
	RegisterFunction("nexell.write_root", WriteRootFn);
}
