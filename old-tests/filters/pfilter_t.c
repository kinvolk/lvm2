/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include "filter-persistent.h"
#include "log.h"
#include "dbg_malloc.h"
#include "config.h"
#include "filter-regex.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char **argv)
{
	struct config_file *cf;
	struct config_node *cn;
	struct dev_filter *rfilter, *pfilter;
	struct dev_iter *iter;
	struct device *dev;

	if (argc < 2) {
		fprintf(stderr, "Usage : %s <regex config>\n",
			argv[0]);
		exit(1);
	}

	init_log(stderr);
	init_debug(_LOG_DEBUG);

	if (!dev_cache_init()) {
		fprintf(stderr, "couldn't initialise dev_cache_init failed\n");
		exit(1);
	}

	if (!dev_cache_add_dir("/dev")) {
		fprintf(stderr, "couldn't add '/dev' to dev_cache\n");
		exit(1);
	}

	if (!(cf = create_config_file())) {
		fprintf(stderr, "couldn't create config file\n");
		exit(1);
	}

	if (!read_config(cf, argv[1])) {
		fprintf(stderr, "couldn't read config file\n");
		exit(1);
	}

	if (!(cn = find_config_node(cf->root, "/devices/filter", '/'))) {
		fprintf(stderr, "couldn't find filter section\n");
		exit(1);
	}

	if (!(rfilter = regex_filter_create(cn->v))) {
		fprintf(stderr, "couldn't build filter\n");
		exit(1);
	}

	if (!(pfilter = persistent_filter_create(rfilter, "./pfilter.cfg"))) {
		fprintf(stderr, "couldn't build filter\n");
		exit(1);
	}

	if (!(iter = dev_iter_create(pfilter))) {
		log_err("couldn't create iterator");
		exit(1);
	}

	fprintf(stderr, "filling cache\n");
	while ((dev = dev_iter_get(iter)))
		;
	dev_iter_destroy(iter);

	fprintf(stderr, "dumping\n");
	if (!persistent_filter_dump(pfilter)) {
		fprintf(stderr, "couldn't dump pfilter\n");
		exit(1);
	}

	fprintf(stderr, "loading\n");
	if (!persistent_filter_load(pfilter)) {
		fprintf(stderr, "couldn't load pfilter\n");
		exit(1);
	}

	if (!(iter = dev_iter_create(pfilter))) {
		log_err("couldn't create iterator");
		exit(1);
	}

	while ((dev = dev_iter_get(iter)))
		printf("%s\n", dev_name(dev));

	dev_iter_destroy(iter);
	pfilter->destroy(pfilter);
	dev_cache_exit();
	destroy_config_file(cf);

	dump_memory();
	fin_log();
	return 0;
}

