#include <vxWorks.h>
#include <stdio.h>

int counter ;
int flag ;

void countRoutine ()
{
   printf ( "Enter countRoutine ()\n" ) ;
   while ( TRUE )
   {
      while ( flag == 1 )
      {
         counter ++ ;
         taskDelay ( 10 ) ;
      }  
   }
}

int menu ()
{
   int choice = 0 ;
   int ret ;

   counter = 0 ;
   flag = 0 ;
   ret = taskSpawn ( "testCount", 100, 0x8, 5000, countRoutine, &counter, &flag, 0, 0, 0, 0, 0, 0, 0, 0 ) ;

   if ( ret == ERROR )
   {
      printf ( "Error when starting taskSpawm\n" ) ;
      return ( ERROR ) ;
   };

   do
   {
     printf ( "\n" ) ;
     printf ( "1 : start counting\n" ) ;
     printf ( "0 : stop counting\n" ) ;
     printf ( "2 : display counter\n" ) ;
     printf ( "-1: quit\n\n" ) ;
     printf ( "    Enter choice : " ) ;
     scanf ( "%d" , &choice ) ;
     printf ( "\n\n" ) ;
     printf ( "choice = %d\n" , choice ) ;
     
     switch ( choice )
     {
       case 1 : flag = 1 ;
                break ;
       case 0 : flag = 0 ;
                break ;
       case 2 : printf ( "counter = %d\n" , counter ) ;
                break ;
       default : flag = 0 ;
                break ;
     }
   } while ( choice != -1 ) ;

   return ( OK ) ;
}
