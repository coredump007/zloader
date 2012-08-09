struct memnode {
	void *mem;

	struct list_head block_list;
	struct list_head list;
};

struct memblock {
	void *mem;

	struct list_head node_head;
	struct list_head list;
};

struct mempool {
	uint32_t size;
	uint32_t n_min;
	uint32_t n_max;

	struct list_head node_head;
	struct list_head block_head;
}

struct mempool *mempool_create(uint32_t size, uint32_t n_min, uint32_t n_max)
{
	struct mempool *mp;
	struct memblock *mb;
	struct memnode *mn;

	void *m;
	uint32_t s;
	int i;

	if (!size || !n_min) {
		E("Invaild arguments.");
		return NULL;
	}


	mp = calloc(1, sizeof(*mp));
	if (!mp) {
		E("fail to create mempool.");
		return NULL;
	}

	mp->size = size;
	mp->n_min = n_min;
	mp->n_max = n_max;

	INIT_LIST_HEAD(&mp->node_head);
	INIT_LIST_HEAD(&mp->block_head);

	mb = calloc(1, sizeof (*mb));
	if (!mb) {
		E("fail to create mempool.");
		goto fail;
	}

	INIT_LIST_HEAD(&mb->list);
	INIT_LIST_HEAD(&mb->node_head);

	mb->mem = NULL;

	s = size * n_min;
	s += sizeof(struct memnode) * n_min;

	m = calloc(1, s);
	if (!m) {
		E("fail to create mempool.");
		goto fail;
	}

	mb->mem = m;

	m += sizeof(struct memnode) * n_min;

	mn = mb->mem;

	for (i = 0; i < n_min; i++) {
		INIT_LIST_HEAD(mn->list);
		INIT_LIST_HEAD(mn->block_list);

		mn->mem = m;

		list_add_tail(&mn->block_list, &mb->node_head);
		list_add_tail(&mn->list, &mp->node_head);

		mn++;
		m += size;
	}

	return 0;
fail:
	if (mp) {
		free(mp);
		mp = NULL;
	}
}

void mempool_destroy(struct mempool *mp, mempool_desc_func_t *)
{

}

struct memnode *mempool_alloc(void)
{

}

void *mempool_free(struct memnode *n)
{

}
