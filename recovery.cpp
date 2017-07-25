#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "edify/expr.h"

static const char *DEVICE = "/dev/block/mmcblk0";

static int UpdateMMC(const char *dev, off_t offset, char *buf, size_t size)
{
	int ret;
	int fd;

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
}
