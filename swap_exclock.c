#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_exclock.h>
#include <list.h>

list_entry_t pra_list_head;

static int
_exclock_init_mm(struct mm_struct *mm)
{
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    return 0;
}

static int
_exclock_map_swappable(struct mm_struct*mm,uintptr_t addr,struct Page*page, int swap_in)
{
    list_entry_t *head = (list_entry_t*)mm->sm_priv;
    list_entry_t *entry = &(page->pra_page_link);

    pte_t* ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
    *ptep &= ~PTE_A;
    *ptep &= ~PTE_D;
    
    list_add(head, entry);
    
    return 0;
}

static int
_exclock_swap_out_victim(struct mm_struct*mm,struct Page**ptr_page, int in_tick)
{
    list_entry_t *head = (list_entry_t*)mm->sm_priv;
    list_entry_t *victim = NULL;
    
	victim = head->prev;
	while(victim != head){//try to find (0,0)
	    struct Page *page = le2page(victim, pra_page_link);
	    pte_t* ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
	    cprintf("addr=%x, (A,D)=(%x,%x)\n",*ptep,(*ptep&PTE_A),(*ptep&PTE_D));
	    if(!(*ptep & PTE_A) && !(*ptep & PTE_D)){
	        *ptr_page = page;
	        list_del(victim);
		return 0;
	    }
	    victim = victim->prev;
	}

	victim = head->prev;
	while(victim != head){//try to find (0,1)
	    struct Page *page = le2page(victim, pra_page_link);
	    pte_t* ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);    
	    if(!(*ptep & PTE_A) && (*ptep & PTE_D)){
		*ptr_page = page;
	        list_del(victim);
		return 0;
	    }
	    victim = victim->prev;
	}

	victim = head->prev;
	while(victim != head){//try to find (1,0)
	    struct Page *page = le2page(victim, pra_page_link);
	    pte_t* ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);    
	    if((*ptep & PTE_A) && !(*ptep & PTE_D)){
		*ptr_page = page;
	        list_del(victim);
		return 0;
	    }
	    victim = victim->prev;
	}

	victim = head->prev;
	while(victim != head){//try to find (1,1)
	    struct Page *page = le2page(victim, pra_page_link);
	    pte_t* ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);    
	    if((*ptep & PTE_A) && (*ptep & PTE_D)){
		*ptr_page = page;
	        list_del(victim);
		return 0;
	    }
	    victim = victim->prev;
	}

    return 0;
}

static int
_exclock_check_swap(void)
{
    cprintf("write Virt Page c in exclock_check_swap.\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==4);
    cprintf("write Virt Page a in exclock_check_swap.\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==4);
    cprintf("write Virt Page d in exclock_check_swap.\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==4);
    cprintf("write Virt Page b in exclock_check_swap.\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==4);
    cprintf("write Virt Page e in exclock_check_swap.\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==5);
    cprintf("write Virt Page b in exclock_check_swap.\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==5);
    cprintf("write Virt Page a in exclock_check_swap.\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==6);
    cprintf("write Virt Page b in exclock_check_swap.\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==7);
    cprintf("write Virt Page c in exclock_check_swap.\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==8);
    cprintf("write Virt Page d in exclock_check_swap.\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==9);
    cprintf("write Virt Page e in exclock_check_swap.\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==10);
    cprintf("write Virt Page a in exclock_check_swap.\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==11);
    return 0;
}

static int
_exclock_init(void){return 0;}

static int
_exclock_set_unswappable(struct mm_struct *mm, uintptr_t addr){return 0;}

static int
_exclock_tick_event(struct mm_struct *mm){return 0;}

struct swap_manager swap_manager_exclock = 
{
    .name = "extended clock swap manager",
    .init = &_exclock_init,
    .init_mm = &_exclock_init_mm,
    .tick_event = &_exclock_tick_event,
    .map_swappable = &_exclock_map_swappable,
    .set_unswappable = &_exclock_set_unswappable,
    .swap_out_victim = &_exclock_swap_out_victim,
    .check_swap = &_exclock_check_swap,
};
