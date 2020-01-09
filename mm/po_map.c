#include <linux/po_metadata.h>
#include <linux/po_map.h>
#include <linux/pmalloc.h>

struct po_vma *po_vma_alloc(size_t len)
{
	struct po_super *super;
	struct po_vma 	*prev_vma, *curr_vma, *new_vma;

	super = po_get_super();
	if (super->vma_free_list_pa == NULL)
		return NULL;
	prev_vma = phys_to_virt(super->vma_free_list_pa);
	curr_vma = prev_vma;
	while (len > curr_vma->size) {
		prev_vma = curr_vma;
		curr_vma = curr_vma->next_pa;
		if (curr_vma == NULL)
			break;
		curr_vma = phys_to_virt(curr_vma);
	}
	if (curr_vma == NULL)
		return NULL;
	if (curr_vma == phys_to_virt(super->vma_free_list_pa)) {
		if (curr_vma->size == len) {
			super->vma_free_list_pa = curr_vma->next_pa;
			return curr_vma;
		} else {
			new_vma = kpmalloc(sizeof(struct po_vma), GFP_KERNEL);
			new_vma->start = curr_vma->start + len;
			new_vma->size = curr_vma->size - len;
			new_vma->next_pa = curr_vma->next_pa;
			super->vma_free_list_pa = virt_to_phys(new_vma);
			curr_vma->size = len;
			return curr_vma;
		}
	} else {
		if (curr_vma->size == len) {
			prev_vma->next_pa = curr_vma->next_pa;
			return curr_vma;
		} else {
			new_vma = kpmalloc(sizeof(struct po_vma), GFP_KERNEL);
			new_vma->start = curr_vma->start + len;
			new_vma->size = curr_vma->size - len;
			new_vma->next_pa = curr_vma->next_pa;
			prev_vma->next_pa = virt_to_phys(new_vma);
			curr_vma->size = len;
			return curr_vma;
		}
	}
}

/*
 * we can build an index in DRAM to speed up the free process.
 */
void po_vma_free(struct po_vma *vma)
{
	struct po_super *super;
	struct po_vma 	*prev_vma, *curr_vma, *tmp;

	// first set next_pa with NULL
	vma->next_pa = NULL;
	super = po_get_super();
	if (super->vma_free_list_pa == NULL) {
		super->vma_free_list_pa = virt_to_phys(vma);
		return;
	}
	prev_vma = phys_to_virt(super->vma_free_list_pa);
	curr_vma = prev_vma;
	while (vma->start > curr_vma->start) {
		prev_vma = curr_vma;
		curr_vma = curr_vma->next_pa;
		if (curr_vma == NULL)
			break;
		curr_vma = phys_to_virt(curr_vma);
	}
	if (curr_vma == NULL) {
		prev_vma->next_pa = virt_to_phys(vma);
	} else {
		if (curr_vma == phys_to_virt(super->vma_free_list_pa)) {
			super->vma_free_list_pa = virt_to_phys(vma);
			vma->next_pa = virt_to_phys(curr_vma);
			prev_vma = vma;	// just for merging conveniently
		} else {
			prev_vma->next_pa = virt_to_phys(vma);
			vma->next_pa = virt_to_phys(curr_vma);
		}
	}
	// try to merge, start from prev_vma
	if (prev_vma->next_pa != NULL) {
		curr_vma = phys_to_virt(prev_vma->next_pa);
		while (prev_vma->start + prev_vma->size == curr_vma->start) {
			prev_vma->size += curr_vma->size;
			tmp = curr_vma;
			curr_vma = curr_vma->next_pa;
			if (curr_vma == NULL) {
				kpfree(tmp);
				break;
			}
			curr_vma = phys_to_virt(curr_vma);
			kpfree(tmp);
		}
		prev_vma->next_pa = curr_vma == NULL ? NULL : virt_to_phys(curr_vma);
	}


}
