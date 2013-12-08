#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lwt.h>
#include <lwt_chan.h>
#include <pthread.h>


__thread int init;


void *
foo(void *data, void* c)
{
	init = (int)data;
	
	sleep(10-init);

	printf("foo %d\n",init);

	return NULL;
}

void *
bar(void *data)
{
	printf("bar %d\n",(int)data);
	
	pthread_detach(pthread_self());
}


int main()
{

	//pthread_t  thd1 = lwt_kthd_create(foo,1,NULL);
	//pthread_t  thd2 = lwt_kthd_create(foo,2,NULL);
/*
	pthread_t thd[10];
	int i = 0,rc;
	
	for(i = 0; i < 8; i++){
		rc = pthread_create(&thd[i],NULL,foo,(void *)i);
		printf("%d \n",rc);
	}

	for(i = 0; i < 8; i++){
		rc = pthread_detach(thd[i]);
		printf("%d \n",rc);
	}
*/


	pthread_t t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	

	int rc = pthread_create(&t,&attr,bar,(void *)10);

	//pthread_detach(t);

	sleep(1);
	printf("main done \n");

	return 0;
}

