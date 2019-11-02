#include<types.h>
#include<mmap.h>

int eqProt(struct vm_area *vm_area, int prot);

u64 endAddr(u64 start_addr, u64 length);

void init_vm_area(struct vm_area *vm_area, struct vm_area *vm_area_next, u64 start, u64 end, int prot);

int insert_vm_area_beginning(struct exec_context *current, u64 addr, int length, int prot, long *ret_addr);

long insert_vm_area_empty_ll(struct exec_context *current, u64 *addr, int length, int prot);

long insert_vm_area_without_hint(struct vm_area *curr_vm_area, struct vm_area *next_vm_area, int length, int prot);

long insert_vm_area_with_fixed_hint(struct vm_area *curr_vm_area, struct vm_area *next_vm_area, u64 addr, int length,
                                    int prot);

long insert_vm_area_with_hint(struct exec_context *current, struct vm_area *curr_vm_area, struct vm_area *next_vm_area,
                              u64 addr, int length, int prot);

void modify_physical_pages(struct exec_context *current, u64 start, u64 end, u32 access_flags);

/**
 * Function will invoked whenever there is page fault. (Lazy allocation)
 *
 * For valid acess. Map the physical page
 * Return 1
 *
 * For invalid access,
 * Return -1.
 */
int vm_area_pagefault(struct exec_context *current, u64 addr, int error_code) {
    int fault_fixed = -1, flag = 0;
    struct vm_area *head_vm_area = current->vm_area;
    void *base_addr;
    base_addr = osmap(current->pgd);

    while (head_vm_area) {
        if (head_vm_area->vm_start <= addr && head_vm_area->vm_end > addr) {
            u64 diff = addr - head_vm_area->vm_start, addr1 =
                    addr - (diff % PAGE_SIZE), prot = head_vm_area->access_flags;
            if (error_code == 0x6) { // unmapped write
                if (head_vm_area->access_flags & PROT_WRITE) {
                    u64 pfn = map_physical_page((u64) base_addr, addr1, prot, 0);
                    fault_fixed = 1;
                }
            } else if (error_code == 0x4) { // unmapped read
                if (head_vm_area->access_flags & PROT_READ || head_vm_area->access_flags & PROT_WRITE) {
                    u64 pfn = map_physical_page((u64) base_addr, addr1, prot, 0);
                    fault_fixed = 1;
                }
            }
            return fault_fixed;
        }
        head_vm_area = head_vm_area->vm_next;
    }
    return fault_fixed;
}

/**
 * mprotect System call Implementation.
 */
int vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot) {
    if (length % PAGE_SIZE) length = (length / PAGE_SIZE + 1) * PAGE_SIZE;
    u64 start = addr, end = addr + length;
    struct vm_area *head_vm_area = current->vm_area, *prev_vm_area = NULL, *next_vm_area = head_vm_area->vm_next, *temp, *temp1;
    while (head_vm_area) {
        if (head_vm_area->vm_start >= end) break;
        if (head_vm_area->access_flags == prot) {
            head_vm_area = head_vm_area->vm_next;
            continue;
        }
        if (head_vm_area->vm_end <= start) {
            head_vm_area = head_vm_area->vm_next;
            continue;
        }
        if ((start <= head_vm_area->vm_start) && (end >= head_vm_area->vm_end)) {
            head_vm_area->access_flags = prot;
            modify_physical_pages(current, head_vm_area->vm_start, head_vm_area->vm_end, prot);
            if (prev_vm_area && eqProt(prev_vm_area, prot) && (prev_vm_area->vm_end == head_vm_area->vm_start)) {
                if (next_vm_area && eqProt(next_vm_area, prot) && (next_vm_area->vm_start == head_vm_area->vm_end)) {
                    prev_vm_area->vm_end = next_vm_area->vm_end;
                    prev_vm_area->vm_next = next_vm_area->vm_next;
                    temp = head_vm_area;
                    temp1 = next_vm_area;
                    head_vm_area = next_vm_area->vm_next;
                    next_vm_area = next_vm_area->vm_next ? next_vm_area->vm_next->vm_next : NULL;
                    dealloc_vm_area(temp);
                    dealloc_vm_area(temp1);
                } else {
                    prev_vm_area->vm_end = head_vm_area->vm_end;
                    prev_vm_area->vm_next = next_vm_area;
                    temp = head_vm_area;
                    head_vm_area = next_vm_area;
                    next_vm_area = next_vm_area ? next_vm_area->vm_next : NULL;
                    dealloc_vm_area(temp);
                }
            } else if (next_vm_area && eqProt(next_vm_area, prot) && (next_vm_area->vm_start == head_vm_area->vm_end)) {
                head_vm_area->vm_end = next_vm_area->vm_end;
                head_vm_area->vm_next = next_vm_area->vm_next;
                temp = next_vm_area;
                prev_vm_area = head_vm_area;
                head_vm_area = next_vm_area->vm_next;
                next_vm_area = next_vm_area->vm_next ? next_vm_area->vm_next->vm_next : NULL;
                dealloc_vm_area(temp);
            } else {
                prev_vm_area = head_vm_area;
                head_vm_area = next_vm_area;
                next_vm_area = next_vm_area ? next_vm_area->vm_next : NULL;
            }
            continue;
        } else if (start <= head_vm_area->vm_start) {
            modify_physical_pages(current, head_vm_area->vm_start, end, prot);
            temp = alloc_vm_area();
            if (stats->num_vm_area == 128) return -EINVAL;
            init_vm_area(temp, head_vm_area->vm_next, end, head_vm_area->vm_end, head_vm_area->access_flags);
            head_vm_area->vm_end = end;
            head_vm_area->vm_next = temp;
            head_vm_area->access_flags = prot;
            head_vm_area = temp;
        } else if (end >= head_vm_area->vm_end) {
            modify_physical_pages(current, start, head_vm_area->vm_end, prot);
            temp = alloc_vm_area();
            if (stats->num_vm_area == 128) return -EINVAL;
            init_vm_area(temp, head_vm_area->vm_next, start, head_vm_area->vm_end, prot);
            head_vm_area->vm_end = start;
            head_vm_area->vm_next = temp;
            head_vm_area = temp;
        } else if ((start > head_vm_area->vm_start) && (end < head_vm_area->vm_end)) {
            modify_physical_pages(current, start, end, prot);
            if (stats->num_vm_area == 127) return -EINVAL;
            temp = alloc_vm_area();
            temp1 = alloc_vm_area();
            init_vm_area(temp, temp1, start, end, prot);
            init_vm_area(temp1, head_vm_area->vm_next, end, head_vm_area->vm_end, head_vm_area->access_flags);
            head_vm_area->vm_end = start;
            head_vm_area->vm_next = temp;
            break;
        }
        head_vm_area = head_vm_area->vm_next;
    }
    if (!head_vm_area) return -EINVAL;
    int isValid = 0;
    return isValid;
}

void unmap_physical_pages(struct exec_context *current, u64 start, u64 end, u32 access_flags) {
    u64 length = end - start;
    for (u64 i = 0; i < length / PAGE_SIZE; i++) {
        u64 addr = start + i * PAGE_SIZE;
        do_unmap_user(current, addr);
    }
}

/**
 * mmap system call implementation.
 */
long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags) {
    if (length % PAGE_SIZE != 0) length = (length / PAGE_SIZE + 1) * PAGE_SIZE;
    long ret_addr = -EINVAL;
    if (current->vm_area == NULL) ret_addr = insert_vm_area_empty_ll(current, &addr, length, prot);
    else if (!addr) {
        if (insert_vm_area_beginning(current, MMAP_AREA_START, length, prot, &ret_addr)) ret_addr = ret_addr;
        else ret_addr = insert_vm_area_without_hint(current->vm_area, current->vm_area->vm_next, length, prot);
    } else if (flags & MAP_FIXED) {
        if (insert_vm_area_beginning(current, addr, length, prot, &ret_addr)) ret_addr = ret_addr;
        else
            ret_addr = insert_vm_area_with_fixed_hint(current->vm_area, current->vm_area->vm_next, addr, length, prot);
    } else {
        if (insert_vm_area_beginning(current, addr, length, prot, &ret_addr)) ret_addr = ret_addr;
        else
            ret_addr = insert_vm_area_with_hint(current, current->vm_area, current->vm_area->vm_next, addr, length,
                                                prot);
    }
    if (ret_addr == -1 || !(flags & MAP_POPULATE)) {
        return ret_addr;
    } else {
        void *base_addr;
        base_addr = osmap(current->pgd);
        for (int i = 0; i < length / PAGE_SIZE; i++) {
            u64 addr1 = addr + i * PAGE_SIZE;
            map_physical_page((u64) base_addr, addr1, prot, 0);
        }
        return ret_addr;
    }
}

long insert_vm_area_with_hint(struct exec_context *current, struct vm_area *curr_vm_area,
                              struct vm_area *next_vm_area, u64 addr, int length, int prot) {
    long ret_addr = -EINVAL;
    struct vm_area *temp_vm_area;
    int cond1, cond2, cond3, cond4;
    while (next_vm_area) {
        if (addr < curr_vm_area->vm_end) {
            ret_addr = insert_vm_area_without_hint(curr_vm_area, next_vm_area, length,
                                                   prot); // overlapping with current curr_vm_area
            if (ret_addr == -EINVAL) {
                if (insert_vm_area_beginning(current, MMAP_AREA_START, length, prot, &ret_addr)) return ret_addr;
                else
                    return insert_vm_area_without_hint(current->vm_area, current->vm_area->vm_next, length,
                                                       prot);
            }
            return ret_addr;
        } else if ((addr >= curr_vm_area->vm_end) && (endAddr(addr, length) <= next_vm_area->vm_start)) {
            cond1 = (addr == curr_vm_area->vm_end);
            cond2 = ((addr + length) == next_vm_area->vm_start);
            cond3 = eqProt(curr_vm_area, prot);
            cond4 = eqProt(next_vm_area, prot);
            if (cond1 && cond2 && cond3 && cond4) {
                curr_vm_area->vm_end = next_vm_area->vm_end;
                curr_vm_area->vm_next = next_vm_area->vm_next;
                dealloc_vm_area(next_vm_area);
                ret_addr = (long) curr_vm_area->vm_start;
            } else if (cond1 && cond3) {
                curr_vm_area->vm_end = endAddr(addr, length);
                ret_addr = (long) curr_vm_area->vm_start;
            } else if (cond2 && cond4) {
                next_vm_area->vm_start = addr;
                ret_addr = (long) addr;
            } else {
                if (stats->num_vm_area == 128) return ret_addr;
                temp_vm_area = alloc_vm_area();
                curr_vm_area->vm_next = temp_vm_area;
                init_vm_area(temp_vm_area, next_vm_area, addr, endAddr(addr, length), prot);
                ret_addr = (long) temp_vm_area->vm_start;
            }
            break;
        } else {
            curr_vm_area = next_vm_area;
            next_vm_area = next_vm_area->vm_next;
        }
    }
    if (next_vm_area == NULL) { // should insert in the end
        if ((addr < curr_vm_area->vm_end) || (endAddr(addr, length) > MMAP_AREA_END)) {
            ret_addr = insert_vm_area_without_hint(curr_vm_area, next_vm_area, length, prot);
            if (ret_addr == -EINVAL) {
                if (insert_vm_area_beginning(current, MMAP_AREA_START, length, prot, &ret_addr)) return ret_addr;
                else
                    return insert_vm_area_without_hint(current->vm_area, current->vm_area->vm_next, length, prot);
            }
            return ret_addr;
        }
        if (eqProt(curr_vm_area, prot) && (addr == curr_vm_area->vm_end)) {
            curr_vm_area->vm_end += length;
            ret_addr = (long) curr_vm_area->vm_start;
            return ret_addr;
        } else {
            if (stats->num_vm_area == 128) return ret_addr;
            temp_vm_area = alloc_vm_area();
            curr_vm_area->vm_next = temp_vm_area;
            init_vm_area(temp_vm_area, NULL, addr, endAddr(addr, length), prot);
            ret_addr = (long) temp_vm_area->vm_start;
            return ret_addr;
        }
    }
}

long insert_vm_area_with_fixed_hint(struct vm_area *curr_vm_area, struct vm_area *next_vm_area, u64 addr, int length,
                                    int prot) {
    long ret_addr = -EINVAL;
    struct vm_area *temp_vm_area;
    int cond1, cond2, cond3, cond4;
    while (next_vm_area) {
        cond1 = (addr < curr_vm_area->vm_end);
        cond2 = (addr >= curr_vm_area->vm_end);
        cond3 = (endAddr(addr, length) <= next_vm_area->vm_start);
        if (cond1) return ret_addr; // overlapping with current curr_vm_area
        else if (cond2 && cond3) {
            cond1 = (addr == curr_vm_area->vm_end);
            cond2 = ((addr + length) == next_vm_area->vm_start);
            cond3 = eqProt(curr_vm_area, prot);
            cond4 = eqProt(next_vm_area, prot);
            if (cond1 && cond2 && cond3 && cond4) {
                curr_vm_area->vm_end = next_vm_area->vm_end;
                curr_vm_area->vm_next = next_vm_area->vm_next;
                dealloc_vm_area(next_vm_area);
                ret_addr = (long) curr_vm_area->vm_start;
            } else if (cond1 && cond3) {
                curr_vm_area->vm_end = endAddr(addr, length);
                ret_addr = (long) curr_vm_area->vm_start;
            } else if (cond2 && cond4) {
                next_vm_area->vm_start = addr;
                ret_addr = (long) addr;
            } else {
                if (stats->num_vm_area == 128) return ret_addr;
                temp_vm_area = alloc_vm_area();
                curr_vm_area->vm_next = temp_vm_area;
                init_vm_area(temp_vm_area, next_vm_area, addr, endAddr(addr, length), prot);
                ret_addr = (long) temp_vm_area->vm_start;
            }
            break;
        } else {
            curr_vm_area = next_vm_area;
            next_vm_area = next_vm_area->vm_next;
        }
    }
    if (next_vm_area == NULL) { // should insert in the end
        cond1 = (addr < curr_vm_area->vm_end);
        cond2 = (endAddr(addr, length) > MMAP_AREA_END);
        cond3 = eqProt(curr_vm_area, prot);
        cond4 = (addr == curr_vm_area->vm_end);
        if (cond1 || cond2) return ret_addr;
        if (cond3 && cond4) {
            curr_vm_area->vm_end += length;
            ret_addr = (long) curr_vm_area->vm_start;
            return ret_addr;
        } else {
            if (stats->num_vm_area == 128) return ret_addr;
            temp_vm_area = alloc_vm_area();
            curr_vm_area->vm_next = temp_vm_area;
            init_vm_area(temp_vm_area, NULL, addr, endAddr(addr, length), prot);
            ret_addr = (long) temp_vm_area->vm_start;
            return ret_addr;
        }
    }
}

long insert_vm_area_without_hint(struct vm_area *curr_vm_area, struct vm_area *next_vm_area, int length, int prot) {
    long ret_addr = -EINVAL;
    u64 start = curr_vm_area->vm_end, end = endAddr(curr_vm_area->vm_end, length);
    struct vm_area *temp_vm_area;
    int cond1, cond2, cond3, cond4;
    while (next_vm_area) {
        start = curr_vm_area->vm_end;
        end = endAddr(curr_vm_area->vm_end, length);
        cond1 = (curr_vm_area->access_flags == prot);
        cond2 = (next_vm_area->access_flags == prot);
        cond3 = (next_vm_area->vm_start == end);
        cond4 = (next_vm_area->vm_start >= end);
        if (cond1 && cond2 && cond3) {
            curr_vm_area->vm_end = next_vm_area->vm_end;
            curr_vm_area->vm_next = next_vm_area->vm_next;
            dealloc_vm_area(next_vm_area);
            ret_addr = (long) curr_vm_area->vm_start;
            break;
        } else if (!cond1 && cond2 && cond3) {
            next_vm_area->vm_start = curr_vm_area->vm_end;
            ret_addr = (long) next_vm_area->vm_start;
            break;
        } else if (cond1 && cond4) {
            curr_vm_area->vm_end = end;
            ret_addr = (long) curr_vm_area->vm_start;
            break;
        } else if (cond4) {
            if (stats->num_vm_area == 128) return ret_addr;
            temp_vm_area = alloc_vm_area();
            curr_vm_area->vm_next = temp_vm_area;
            init_vm_area(temp_vm_area, next_vm_area, start, end, prot);
            ret_addr = (long) temp_vm_area->vm_start;
            break;
        } else {
            curr_vm_area = next_vm_area;
            next_vm_area = next_vm_area->vm_next;
        }
    }
    if (next_vm_area == NULL) { // should insert in the end
        cond1 = eqProt(curr_vm_area, prot);
        cond2 = (MMAP_AREA_END >= end);
        cond3 = (MMAP_AREA_END >= end);
        if (cond1 && cond2) {
            curr_vm_area->vm_end += length;
            ret_addr = (long) curr_vm_area->vm_start;
            return ret_addr;
        } else if (cond3) {
            if (stats->num_vm_area == 128) return ret_addr;
            temp_vm_area = alloc_vm_area();
            curr_vm_area->vm_next = temp_vm_area;
            init_vm_area(temp_vm_area, NULL, start, end, prot);
            ret_addr = (long) temp_vm_area->vm_start;
            return ret_addr;
        } else return ret_addr; // cannot insert anywhere
    }
}

long insert_vm_area_empty_ll(struct exec_context *current, u64 *addr, int length, int prot) {
    *addr = (*addr ? *addr : MMAP_AREA_START);
    u64 end = endAddr(*addr, length);
    if (end > MMAP_AREA_END) return -EINVAL;
    if (stats->num_vm_area == 128) return -EINVAL;
    current->vm_area = alloc_vm_area();
    init_vm_area(current->vm_area, NULL, *addr, end, prot);
    return *addr;
}

int insert_vm_area_beginning(struct exec_context *current, u64 addr, int length, int prot, long *ret_addr) {
    u64 start = addr, end = endAddr(addr, length);
    if (eqProt(current->vm_area, prot) && (current->vm_area->vm_start == end)) {
        // merging in the beginning
        current->vm_area->vm_start = addr;
        *ret_addr = addr;
        return 1;
    } else if (current->vm_area->vm_start >= end) {
        if (stats->num_vm_area == 128) return -EINVAL;
        struct vm_area *temp_vm_area = alloc_vm_area();
        init_vm_area(temp_vm_area, current->vm_area, start, end, prot);
        current->vm_area = temp_vm_area;
        *ret_addr = addr;
        return 1;
    } else return 0;
}

int eqProt(struct vm_area *vm_area, int prot) {
    return (vm_area->access_flags == prot);
}

u64 endAddr(u64 start_addr, u64 length) {
    return start_addr + length;
}

void init_vm_area(struct vm_area *vm_area, struct vm_area *vm_area_next, u64 start, u64 end, int prot) {
    vm_area->vm_start = start;
    vm_area->vm_end = end;
    vm_area->access_flags = prot;
    vm_area->vm_next = vm_area_next;
}

void modify_physical_pages(struct exec_context *current, u64 start, u64 end, u32 access_flags) {
    u64 length = end - start;
    void *base_addr = osmap(current->pgd);
    for (int i = 0; i < length / PAGE_SIZE; i++) {
        u64 addr = start + i * PAGE_SIZE;
        u64 *pte = get_user_pte(current, addr, 0);
        if (!pte) {
            continue;
        } else {
            if (access_flags & PROT_WRITE) {
                *pte |= PROT_WRITE;
            } else {
                if (*pte & PROT_WRITE) {
                    *pte ^= PROT_WRITE;
                }
            }
        }

        asm volatile ("invlpg (%0);"
        ::"r"(addr)
        : "memory");   // Flush TLB
    }
}

/**
 * munmap system call implemenations
 */

int vm_area_unmap(struct exec_context *current, u64 addr, int length) {
    if (length % PAGE_SIZE) length = (length / PAGE_SIZE + 1) * PAGE_SIZE;
    u64 start = addr, end = addr + length;
    struct vm_area *prev_vm_area = NULL, *head_vm_area = current->vm_area, *temp;
    while (head_vm_area) {
        if (head_vm_area->vm_start >= end) break;
        if (head_vm_area->vm_end <= start) {
            prev_vm_area = head_vm_area;
            head_vm_area = head_vm_area->vm_next;
            continue;
        }
        if ((start <= head_vm_area->vm_start) && (end >= head_vm_area->vm_end)) {
            if (prev_vm_area) {
                prev_vm_area->vm_next = head_vm_area->vm_next;
            } else {
                current->vm_area = head_vm_area->vm_next;
            }
            temp = head_vm_area;
            unmap_physical_pages(current, head_vm_area->vm_start, head_vm_area->vm_end, head_vm_area->access_flags);
            head_vm_area = head_vm_area->vm_next;
            dealloc_vm_area(temp);
            continue;
        } else if (start <= head_vm_area->vm_start) {
            unmap_physical_pages(current, head_vm_area->vm_start, end, head_vm_area->access_flags);
            head_vm_area->vm_start = end;
        } else if (end >= head_vm_area->vm_end) {
            unmap_physical_pages(current, start, head_vm_area->vm_end, head_vm_area->access_flags);
            head_vm_area->vm_end = start;
        } else if ((start > head_vm_area->vm_start) && (end < head_vm_area->vm_end)) {
            unmap_physical_pages(current, start, end, head_vm_area->access_flags);
            if (stats->num_vm_area == 128) return -EINVAL;
            temp = alloc_vm_area();
            init_vm_area(temp, head_vm_area->vm_next, end, head_vm_area->vm_end, head_vm_area->access_flags);
            head_vm_area->vm_end = start;
            head_vm_area->vm_next = temp;
        }
        prev_vm_area = head_vm_area;
        head_vm_area = head_vm_area->vm_next;
    }
    int isValid = 0;
    return isValid;
}
