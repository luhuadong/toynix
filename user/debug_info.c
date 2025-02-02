#include <lib.h>

void
usage(void)
{
	cprintf("usage: debug_info cpu/mem/fs\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i, fd, ret, envid;
	char buf[128] = {0};
	uint32_t *tmp = UTEMP;
	struct vm_area_struct *vma;
	const char *env_status[] = {
		"free",
		"dying",
		"waiting",
		"running",
		"pending",
	};

	if (argc == 1)
		usage();

	for (i = 0; i < MAXDEBUGOPT; i++) {
		ret = strcmp(argv[1], debug_option[i]);
		if (!ret)
			break;
	}

	switch (i) {
	case CPU_INFO:
	case MEM_INFO:
		fd = opendebug();
		if (fd < 0)
			return;

		write(fd, argv[1], strlen(argv[1]));
		read(fd, buf, sizeof(buf));

		close(fd);
		break;

	case FS_INFO:
		ret = sys_page_alloc(0, tmp, PTE_W);

		ipc_send(ipc_find_env(ENV_TYPE_FS), FSREQ_INFO, tmp, PTE_W);
		ret = ipc_recv(NULL, NULL, NULL);
		if (ret < 0)
			return;

		snprintf(buf, sizeof(buf),
				"Total Blocks Num: %d\n"
				"Used Blocks Num: %d\n",
				tmp[0], tmp[1]);

		sys_page_unmap(0, tmp);
		break;

	case ENV_INFO:
		for (i = 0; i < NENV; i++) {
			if (envs[i].env_status != ENV_FREE) {
				printf("Env: %x Name: %16s Status: %8s Run Times: %8d Father: %x",
					envs[i].env_id, envs[i].binaryname,
					env_status[envs[i].env_status], envs[i].env_runs,
					envs[i].env_parent_id);
				if (envs[i].env_parent_id)
					printf(" %16s", envs[ENVX(envs[i].env_parent_id)].binaryname);
				printf("\n");
			}
		}
		break;

	case VMA_INFO:
		envid = strtol(argv[2], NULL, 16);

		if (envs[ENVX(envid)].env_status == ENV_FREE)
			return;

		printf("Env %x:\n", envid);
		for (i = 0; i < envs[ENVX(envid)].vma_valid; i++) {
			vma = (void *)&(envs[ENVX(envid)].vma[i]);
			printf("VMA%d Begin: %08x Size: %8x Perm: %x\n",
				i, vma->vm_start, vma->size, vma->vm_page_prot);
		}
		break;

	default:
		return;
	}

	printf("%s", buf);
	return;
}