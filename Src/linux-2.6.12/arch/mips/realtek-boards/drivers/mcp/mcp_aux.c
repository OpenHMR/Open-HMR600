#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>



int mcp_is_pli_address(void* addr)
{
    struct vm_area_struct* vma; 
  
    vma = find_extend_vma(current->mm, (unsigned long) addr);
        
    return (vma && (vma->vm_flags & VM_DVR)) ? 1 : 0;    
}



#define pli_to_kernel(addr)           ((void*) (((u32)addr & 0x3FFFFFFF) | 0x80000000))
