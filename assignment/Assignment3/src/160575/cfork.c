#include <cfork.h>
#include <page.h>
#include <mmap.h>

void vm_area_init(struct vm_area *vm_area, struct vm_area *vm_area_next, u64 start, u64 end, int prot);

/* You need to implement cfork_copy_mm which will be called from do_cfork in entry.c. Don't remove copy_os_pts()*/
void cfork_copy_mm(struct exec_context *child, struct exec_context *parent) {

    struct mm_segment *code_seg;
    void *os_addr;
    u64 var_addr;
    child->pgd = os_pfn_alloc(OS_PT_REG);
    os_addr = osmap(child->pgd);
    bzero((char *) os_addr, PAGE_SIZE);
    code_seg = &parent->mms[MM_SEG_CODE]; //CODE segment
    for (var_addr = code_seg->start; var_addr < code_seg->next_free; var_addr += PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            u64 pfn = install_ptable((u64) os_addr, code_seg, var_addr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
            struct pfn_info *p = get_pfn_info(pfn);
            increment_pfn_info_refcount(p);
        }
    }
    code_seg = &parent->mms[MM_SEG_RODATA]; //RODATA segment
    for (var_addr = code_seg->start; var_addr < code_seg->next_free; var_addr += PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            u64 pfn = install_ptable((u64) os_addr, code_seg, var_addr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
            struct pfn_info *p = get_pfn_info(pfn);
            increment_pfn_info_refcount(p);
        }
    }
    code_seg = &parent->mms[MM_SEG_DATA]; //DATA segment
    for (var_addr = code_seg->start; var_addr < code_seg->next_free; var_addr += PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            *parent_pte ^= PROT_WRITE;
            code_seg->access_flags = PROT_READ;
            u64 pfn = install_ptable((u64) os_addr, code_seg, var_addr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
            struct pfn_info *p = get_pfn_info(pfn);
            increment_pfn_info_refcount(p);
            code_seg->access_flags = PROT_WRITE | PROT_READ;
        }
    }
    code_seg = &parent->mms[MM_SEG_STACK]; //STACK segment
    for (var_addr = code_seg->end - PAGE_SIZE; var_addr >= code_seg->next_free; var_addr -= PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            u64 pfn = install_ptable((u64) os_addr, code_seg, var_addr, 0);  //Returns the blank page
            pfn = (u64) osmap(pfn);
            memcpy((char *) pfn, (char *) (*parent_pte & FLAG_MASK), PAGE_SIZE);
        }
    }
    struct vm_area *head_vm_area = parent->vm_area;
    struct vm_area *prev = NULL;
    while (head_vm_area) {
        u64 start = head_vm_area->vm_start, end = head_vm_area->vm_end, length = (end - start) / PAGE_SIZE;
        struct vm_area *next_vm_area = alloc_vm_area();
        if (prev) prev->vm_next = next_vm_area;
        else {
            prev = next_vm_area;
            child->vm_area = prev;
        }
        vm_area_init(next_vm_area, NULL, head_vm_area->vm_start, head_vm_area->vm_end, head_vm_area->access_flags);
        for (int i = 0; i < length; i++) {
            u64 addr = start + i * PAGE_SIZE;
            u64 *parent_pte = get_user_pte(parent, addr, 0);
            u32 access_flags = PROT_READ;
            if (parent_pte) {
                if (*parent_pte & PROT_WRITE) *parent_pte ^= PROT_WRITE;
                u64 pfn = map_physical_page((u64) os_addr, addr, access_flags, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
                increment_pfn_info_refcount(get_pfn_info(pfn));
            }
        }
        prev = next_vm_area;
        head_vm_area = head_vm_area->vm_next;
    }
    copy_os_pts(parent->pgd, child->pgd);
}

/* You need to implement cfork_copy_mm which will be called from do_vfork in entry.c.*/
void vfork_copy_mm(struct exec_context *child, struct exec_context *parent) {

    struct mm_segment *code_seg;
    void *os_addr;
    u64 var_addr;
    child->pgd = os_pfn_alloc(OS_PT_REG);
    os_addr = osmap(child->pgd);
    bzero((char *) os_addr, PAGE_SIZE);
    code_seg = &parent->mms[MM_SEG_CODE];
    for (var_addr = code_seg->start; var_addr < code_seg->next_free; var_addr += PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            install_ptable((u64) os_addr, code_seg, var_addr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
        }
    }
    code_seg = &parent->mms[MM_SEG_RODATA];
    for (var_addr = code_seg->start; var_addr < code_seg->next_free; var_addr += PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte)
            install_ptable((u64) os_addr, code_seg, var_addr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
    }
    code_seg = &parent->mms[MM_SEG_DATA];
    for (var_addr = code_seg->start; var_addr < code_seg->next_free; var_addr += PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            install_ptable((u64) os_addr, code_seg, var_addr, (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
        }
    }
    code_seg = &parent->mms[MM_SEG_STACK];
    for (var_addr = code_seg->end - PAGE_SIZE; var_addr >= code_seg->next_free; var_addr -= PAGE_SIZE) {
        u64 *parent_pte = get_user_pte(parent, var_addr, 0);
        if (parent_pte) {
            u64 pfn = install_ptable((u64) os_addr, code_seg, var_addr, 0);  //Returns the blank page
            pfn = (u64) osmap(pfn);
            memcpy((char *) pfn, (char *) (*parent_pte & FLAG_MASK), PAGE_SIZE);
        }
    }
    struct vm_area *head_vm_area = parent->vm_area;
    u64 start, end, length, addr;
    while (head_vm_area) {
        start = head_vm_area->vm_start;
        end = head_vm_area->vm_end;
        length = (end - start) / PAGE_SIZE;
        for (int i = 0; i < length; i++) {
            addr = start + i * PAGE_SIZE;
            u64 *parent_pte = get_user_pte(parent, addr, 0);
            if (parent_pte) {
                map_physical_page((u64) os_addr, addr, head_vm_area->access_flags,
                                  (*parent_pte & FLAG_MASK) >> PAGE_SHIFT);
            }
        }
        head_vm_area = head_vm_area->vm_next;
    }
    parent->state = WAITING;
    copy_os_pts(parent->pgd, child->pgd);
}

/*You need to implement handle_cow_fault which will be called from do_page_fault
incase of a copy-on-write fault

* For valid acess. Map the physical page
 * Return 1
 *
 * For invalid access,
 * Return -1.
*/

int handle_cow_fault(struct exec_context *current, u64 cr2) {
    void *os_addr;
    os_addr = osmap(current->pgd);
    struct mm_segment *code_seg;
    code_seg = &current->mms[MM_SEG_DATA];
    u64 start = code_seg->start, end = code_seg->end, *pte = get_user_pte(current, cr2, 0), pfn =
            (*pte & FLAG_MASK) >> PAGE_SHIFT;
    struct pfn_info *p = get_pfn_info(pfn);
    u64 ref_count = get_pfn_info_refcount(p);

    if (cr2 >= start && cr2 < end) {
        if (ref_count > 1) {
            u64 pfn1 = install_ptable((u64) os_addr, code_seg, cr2, 0);  //Returns the blank page
            decrement_pfn_info_refcount(p);
            pfn1 = (u64) osmap(pfn1);
            memcpy((char *) pfn1, (char *) (*pte & FLAG_MASK), PAGE_SIZE);
            return 1;
        } else {
            if (PROT_WRITE & code_seg->access_flags) {
                *pte = *pte | PROT_WRITE;
                return 1;
            } else {
                return -1;
            }
        }
    }
    struct vm_area *head_vm_area = current->vm_area;
    int flag = 0;
    while (head_vm_area) {
        if (head_vm_area->vm_end > cr2 && head_vm_area->vm_start <= cr2) {
            if (head_vm_area->access_flags & PROT_WRITE) {
                if (ref_count > 1) {
                    u64 pfn1 = map_physical_page((u64) os_addr, cr2, PROT_WRITE, 0);
                    decrement_pfn_info_refcount(p);
                    pfn1 = (u64) osmap(pfn1);
                    memcpy((char *) pfn1, (char *) (*pte & FLAG_MASK), PAGE_SIZE);
                    return 1;
                } else {
                    *pte = *pte | PROT_WRITE;
                    return 1;
                }
            } else {
                return -1;
            }
        }
        head_vm_area = head_vm_area->vm_next;
    }
    return -1;
}

/* You need to handle any specific exit case for vfork here, called from do_exit*/
void vfork_exit_handle(struct exec_context *ctx) {
    u64 parent_pid = ctx->ppid;
    if (parent_pid) {
        struct exec_context *parent_ctx = get_ctx_by_pid(parent_pid);
        if (parent_ctx->state == WAITING) parent_ctx->state = READY;
    }
}

void vm_area_init(struct vm_area *vm_area, struct vm_area *vm_area_next, u64 start, u64 end, int prot) {
    vm_area->vm_start = start;
    vm_area->vm_end = end;
    vm_area->access_flags = prot;
    vm_area->vm_next = vm_area_next;
}