#include <stdio.h>
#include <stdlib.h>
#include "../include/list.h"
#include "../include/utilities.h"

typedef struct ListNode
{
    void* value;
    struct ListNode* next;
}
ListNode;
typedef struct ListNode* list_node_ptr;

typedef struct listSet
{
    list_node_ptr top;    // top node of the list
    size_t size;          // the number of elements stored in the list
    DestroyFunc destroy;  // function that destroys the data, NULL if want to preserve the data
}
listSet;

List list_create(const DestroyFunc destroy)
{
    List list = custom_malloc(sizeof(*list));
    list->size = 0;
    list->destroy = destroy;
    list->top = NULL;
    return list;
}

void* list_top_value(const List list)  { return (list->top->value); }

size_t list_size(const List list)  { return list->size; }

void list_push(const List list, void* value)
{
    list_node_ptr* head = &(list->top);

    // create a new node
    list_node_ptr node = custom_malloc(sizeof(ListNode));

    node->value = value;
    node->next = *head;
    *head = node;

    list->size++;  // element was inserted
}

size_t list_destroy(const List list)
{
    size_t bytes_freed = 0;
    list_node_ptr head = list->top;

    while (head != NULL)
    {
        const list_node_ptr tmp = head;
        head = head->next;

        // if a destoy function was given destroy the data and count the bytes destroyed
        if (list->destroy != NULL)
            bytes_freed += list->destroy(tmp->value);

        bytes_freed += sizeof(*tmp);
        free(tmp);
    }

    bytes_freed += sizeof(*list);
    free(list);
    return bytes_freed;
}

// use the merge sort algorithm to sort the list - O(nlogn)
// source: https://en.wikipedia.org/wiki/Merge_sort#Top-down_implementation_using_lists

// split the list into two halves, placing the front part in front and the back part in back
static inline void list_split(const list_node_ptr source, list_node_ptr* front, list_node_ptr* back)
{
    // base case: list is empty or it has 1 element
    if (source == NULL || source->next == NULL)
    {
        *back = NULL;
        *front = source;
        return;
    } 

    // use a s & f pointer to find the middle of the list and split it into two parts
    list_node_ptr s = source, f = source->next;
    while (f != NULL)
    {
        f = f->next;
        if (f != NULL)
        {
            s = s->next;
            f = f->next;
        }
    }

    *back = s->next;
    s->next = NULL;
    *front = source;
}

// recursively merge the lists l_list and r_list into one sorted list
static list_node_ptr merge(const list_node_ptr l_list, const list_node_ptr r_list, const CompareFunc compare)
{
    // the right list is empty, no need to compare, just return the left one
    if (r_list == NULL) return l_list;

    // the left list is empty, no need to compare, just return the right one
    else if (l_list == NULL) return r_list;

    // compare the two lists using the compare function
    if (compare(l_list->value, r_list->value) > 0)
    {
        r_list->next = merge(l_list, r_list->next, compare);
        return r_list;
    }
    else
    {
        l_list->next = merge(l_list->next, r_list, compare);
        return l_list;
    }
}

static void merge_sort(list_node_ptr* head, const CompareFunc compare)
{
    // base case: split operation can not continue: all elements are sorted
    // const list_node_ptr curr = *head;
    if (*head == NULL || (*head)->next == NULL) return;

    // create two sub-lists and split them    
    list_node_ptr left, right;
    list_split(*head, &left, &right);

    // after splitting is complete, merge the lists
    merge_sort(&left, compare);
    merge_sort(&right, compare);
    *head = merge(left, right, compare);
}

void list_sort(const List list, const CompareFunc compare)
{
    // call merge sort to recursively sort the list
    merge_sort(&list->top, compare);
}

//////////////////////////////////////////////////////
// custom functions
/////////////////////////////////////////////////////

postcode allocate_new_postcode(const int p)
{
    const postcode new_post = custom_malloc(sizeof(*new_post));
    new_post->postcode = p;
    new_post->voters = list_create(NULL);
    return new_post;
}

void list_insert_postcode(const List list, const voter v)
{
    // check if the postcode already exists in the list
    for (list_node_ptr tmp = list->top; tmp != NULL; tmp = tmp->next)
    {
        const postcode a = (postcode)tmp->value;
        if (a->postcode == v->TK)
        {
            list_push(a->voters, v);
            return;
        }
    }

    // postcode does not exist, create it then insert it
    list_push(list, allocate_new_postcode(v->TK));
    const postcode a = (postcode)list_top_value(list);
    list_push(a->voters, v);
}

void print_zipcodes(const List list, const int zipcode)
{
    // check if the postcode already exists in the list
    for (list_node_ptr tmp = list->top; tmp != NULL; tmp = tmp->next)
    {
        const postcode a = (postcode)tmp->value;
        if (a->postcode == zipcode)
        {
            // get the list of voters
            const List zip_voters = a->voters;

            printf("%ld voted in %d\n", list_size(zip_voters), zipcode);

            // print the voters
            for (list_node_ptr v_list = zip_voters->top; v_list != NULL; v_list = v_list->next)
            {
                const voter vot = v_list->value;
                printf("%d\n", vot->PIN);
            }
            return;
        }
    }
    printf("\n");
}

void sorted_print(const List list)
{
    for (list_node_ptr tmp = list->top; tmp != NULL; tmp = tmp->next)
    {
        const postcode a = (postcode)tmp->value;
        printf("%d %ld\n", a->postcode, list_size(a->voters));
    }
    printf("\n");
}
