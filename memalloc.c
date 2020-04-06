#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

union ublockheader {
	struct {
		size_t size;
		bool is_free;
		union ublockheader *next;
	} s;
	char align[16];
};

struct test
{
	int a;
	int b;
} sample;

typedef union ublockheader blockHeader;

blockHeader *head, *tail;

pthread_mutex_t lock;

blockHeader *get_free_memblock(size_t size);

void *myMalloc(size_t size)
{
	blockHeader *header;
	
	if(!size)
		return NULL;
	
	pthread_mutex_lock(&lock);
	
	header = get_free_memblock(size);
	
	if(header)
	{
		header->s.is_free = 0;
	
		pthread_mutex_unlock(&lock);
	
		return (void *)(header + 1); 
	}
	
	int total_size = size + sizeof(header);
	
	void *block = sbrk(total_size);
	
	header = block;
	
	header->s.size = size;
	
	header->s.is_free = 0;
	
	if(!head)
		head = tail = header;
	else
	{
		tail->s.next = header;
	
		tail = header;
	}
	
	pthread_mutex_unlock(&lock);
	
	return (void *)(header + 1);
}

blockHeader *get_free_memblock(size_t size)
{
	blockHeader *curr = head;
	while(curr)
	{
		if(curr->s.size >= size && curr->s.is_free)
			return curr;
		curr = curr->s.next;
	}
	return NULL;
}

void myFree(void *addr)
{
	if(!addr)
		return;

	pthread_mutex_lock(&lock);

	void *heapTop = sbrk(0);

	blockHeader* header = (blockHeader *)addr - 1;

	if(header->s.is_free)
	{
		pthread_mutex_unlock(&lock);
		
		printf("\nError !! Don't try to free block already freed !!\n");
		
		return;
	}

	blockHeader* temp;

	int total_size = header->s.size + sizeof(header);

	if((char *)header + total_size == heapTop)
	{
		printf("\n== yes true, header at %8p , total_size is %4d , program break at %8p == \n", header, total_size, heapTop);
		
		if(head == tail)
		{
			head = tail = NULL;
			
			sbrk(-1 * total_size);
			
			pthread_mutex_unlock(&lock);

			return;
		}
		if(head == header)
		{
			temp = head->s.next;
			
			head = temp;
		}
		else
		{
			temp = head;
			
			while(temp)
			{	
				if(temp->s.next == header)
					break;

				temp = temp->s.next;
			}

			if(tail == header)
			{
				tail = temp;
				tail->s.next = NULL;
			}
			else
				temp->s.next = header->s.next;
		}

		sbrk(-1 * total_size);
	}
	else
	{
		printf("\n== not true, header at %8p , total_size is %4d , program break at %8p == \n", header, total_size, heapTop);

		header->s.is_free = 1;
	}

	pthread_mutex_unlock(&lock);
}

int main()
{

	int i = 0;

	struct test *b = myMalloc(20 * sizeof(sample));

	for(; i < 20; i++)
	{
		struct test *a = myMalloc(sizeof(sample));
		myFree(a);
		b[i].a = i;
		b[i].b = i;
	}	

	myFree(b);

	myFree(b);

	return 0;
}