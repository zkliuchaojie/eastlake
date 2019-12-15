#include <linux/po_metadata.h>
#include <linux/po_map.h>
#include <linux/pmalloc.h>

struct po_vma *po_vma_alloc(size_t len)
{
	struct po_super *super;
	struct po_vma 	*pre_vma, *curr_vma, *new_vma;	

	super = po_get_super();
	if (super->vma_free_list_pa == NULL)
		return NULL;
	pre_vma = phys_to_virt(super->vma_free_list_pa);
	curr_vma = pre_vma;
	while (len > curr_vma->size) {
		pre_vma = curr_vma;
		curr_vma = curr_vma->next_pa;
		if (curr_vma == NULL)
			break;
		curr_vma = phys_to_virt(curr_vma);
	}
	if (curr_vma == NULL)
		return NULL;
	if (curr_vma == phys_to_virt(super->vma_free_list_pa)) {
		if (curr->size == len) {
			super->vma_free_list_pa = curr_vma->next_pa;
			return curr_vma;
		} else {
			new_vma = kpmalloc(sizeof(struct po_vma));
			new_vma->start = curr_vma->start - len;
			new_vma->size = curr_vma->size - len;
			new_vma->next_pa = curr_vma->next_pa;
			super->vma_free_list_pa = virt_to_phys(new_vma);
			curr_vma->size = len;
			return curr_vma;
		}
	} else {
		if (curr->size == len) {
			pre_vma->next_pa = curr_vma->next_pa;
			return curr_vma;
		} else {
			new_vma = kpmalloc(sizeof(struct po_vma));
			new_vma->start = curr_vma->start - len;
			new_vma->size = curr_vma->size - len;
			new_vma->next_pa = curr_vma->next_pa;
			pre_vma->next_pa = virt_to_phys(new_vma);
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
	struct po_vma 	*pre_vma, *curr_vma, *tmp;

	// first set next_pa with NULL
	vma->next_pa = NULL;
	super = po_get_super();
	if (super->vma_free_list_pa == NULL) {
		super->vma_free_list_pa = virt_to_phys(vma);
		return;
	}
	pre_vma = phys_to_virt(super->vma_free_list_pa);
	curr_vma = pre_vma;
	while (vma->start > curr_vma->start) {
		pre_vma = curr_vma;
		curr_vma = curr_vma->next_pa;
		if (curr_vma == NULL)
			break;
		curr_vma = phys_to_virt(curr_vma);
	}
	if (curr_vma == NULL) {
		pre_vma->next_pa = virt_to_phys(vma);
	} else {
		if (curr_vma == phys_to_virt(super->vma_free_list_pa))
			super->vma_free_list_pa = virt_to_phys(vma);
			vma->next_pa = virt_to_phys(curr_vma);
			pre_vma = vma;	// just for merging conveniently
		} else {
			pre_vma->next_pa = virt_to_phys(vma);
			vma->next_pa = virt_to_phys(curr_vma);
		}
	}
	// try to merge, start from pre_vma
	if (pre_vma->next_pa != NULL) {
		curr_vma = phys_to_virt(pre_vma->next_pa);
		while (pre_vma->start + pre_vma->size == curr_vma->start) {
			pre_vma->size += curr_vma->size;
			tmp = curr_vma;
			curr_vma = curr_vma->next_pa;
			if (curr_vma == NULL) {
				kpfree(tmp);
				break;
			}
			curr_vma = phys_to_virt(curr_vma);
			kpfree(tmp);
		}
		pre_vma->next_pa = curr_vma == NULL ? NULL : virt_to_phys(curr_vma);
	}


}
