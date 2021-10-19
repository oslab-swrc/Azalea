// SPDX-FileCopyrightText: Copyright(c)2008 All rights reserved by kkamagui
//
// SPDX-License-Identifier: MIT

/**
 *  file    ImageMaker.c
 *  date    2008/12/16
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui
 *  brief   부트 로더와 커널 이미지를 연결하고, 섹터 단위로 정렬해 주는 ImageMaker의 
 *          소스 파일
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#define BYTESOFSECTOR  512

// 함수 선언
int AdjustInSectorSize( int iFd, int iSourceSize );
void WriteKernelInformation( int iTargetFd, int iTotalKernelSectorCount,
 		int iKernel32SectorCount,int iPageTableCount, int kernel64_sector_count);
int CopyFile( int iSourceFd, int iTargetFd );

/**
 *  Main 함수
*/
int main(int argc, char* argv[])
{
    int iSourceFd;
    int iTargetFd;
    int iBootLoaderSize;
    int iKernel32SectorCount;
    int iPageTableSize;
    int iKernel64SectorCount;
    int uthread_sector_count;
    int iSourceSize;
        
    // 커맨드 라인 옵션 검사
    if( argc < 5 )
    {
        fprintf( stderr, "[ERROR] qimageMaker bootLoader.bin kernel32.bin page.bin kernel64.bin\n" );
        exit( -1 );
    }
    
    // Disk.img 파일을 생성
    if( ( iTargetFd = open( "qdisk.img", O_RDWR | O_CREAT |  O_TRUNC
            , S_IREAD | S_IWRITE ) ) == -1 )
    {
        fprintf( stderr , "[ERROR] Disk.img open fail.\n" );
        exit( -1 );
    }

    //--------------------------------------------------------------------------
    // 부트 로더 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
    //--------------------------------------------------------------------------
    printf( "[INFO] Copy boot loader to image file\n" );
    if( ( iSourceFd = open( argv[ 1 ], O_RDONLY ) ) == -1 )
    {
        fprintf( stderr, "[ERROR] %s open fail\n", argv[ 1 ] );
        exit( -1 );
    }

    iSourceSize = CopyFile( iSourceFd, iTargetFd );
    close( iSourceFd );
    
    // 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x00 으로 채움
    iBootLoaderSize = AdjustInSectorSize( iTargetFd , iSourceSize );
    printf( "[INFO] %s size = [%d] and sector count = [%d]\n",
            argv[ 1 ], iSourceSize, iBootLoaderSize );

    //--------------------------------------------------------------------------
    // 32비트 커널 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
    //--------------------------------------------------------------------------
    printf( "[INFO] Copy protected mode kernel to image file\n" );
    if( ( iSourceFd = open( argv[ 2 ], O_RDONLY ) ) == -1 )
    {
        fprintf( stderr, "[ERROR] %s open fail\n", argv[ 2 ] );
        exit( -1 );
    }

    iSourceSize = CopyFile( iSourceFd, iTargetFd );
    close( iSourceFd );
    
    // 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x00 으로 채움
    iKernel32SectorCount = AdjustInSectorSize( iTargetFd, iSourceSize );
    printf( "[INFO] %s size = [%d] and sector count = [%d]\n",
                argv[ 2 ], iSourceSize, iKernel32SectorCount );


    //--------------------------------------------------------------------------
    // 64비트 페이지테이블
    //--------------------------------------------------------------------------
    printf( "[INFO] Copy pagetable to image file\n" );
    if( ( iSourceFd = open( argv[ 3 ], O_RDONLY ) ) == -1 )
    {
        fprintf( stderr, "[ERROR] %s open fail\n", argv[ 2 ] );
        exit( -1 );
    }

    iSourceSize = CopyFile( iSourceFd, iTargetFd );
    close( iSourceFd );

    // 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x00 으로 채움
    iPageTableSize = AdjustInSectorSize( iTargetFd, iSourceSize );
    printf( "[INFO] %s size = [%d] and sector count = [%d]\n",
                argv[ 3 ], iSourceSize, iPageTableSize );

    //--------------------------------------------------------------------------
    // 64비트 커널 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
    //--------------------------------------------------------------------------
    printf( "[INFO] Copy IA-32e mode kernel to image file\n" );
    if( ( iSourceFd = open( argv[ 4 ], O_RDONLY ) ) == -1 )
    {
        fprintf( stderr, "[ERROR] %s open fail\n", argv[ 3 ] );
        exit( -1 );
    }

    iSourceSize = CopyFile( iSourceFd, iTargetFd );
    close( iSourceFd );

    // 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x00 으로 채움
    iKernel64SectorCount = AdjustInSectorSize( iTargetFd, iSourceSize );
    printf( "[INFO] %s size = [%d] and sector count = [%d]\n",
                argv[4], iSourceSize, iKernel64SectorCount );

/*
	printf("[INFO] Copying service threads to image file\n");
	if ( ( iSourceFd = open( argv[4], O_RDONLY ) ) == -1 )
	{
        fprintf( stderr, "[ERROR] %s open fail\n", argv[ 4 ] );
        exit( -1 );
	}
	iSourceSize = CopyFile( iSourceFd, iTargetFd );
	close( iSourceFd );

	uthread_sector_count = AdjustInSectorSize( iTargetFd, iSourceSize );
    printf( "[INFO] %s size = [%d] and sector count = [%d]\n",
                argv[ 4 ], iSourceSize, uthread_sector_count );

*/
    printf("[INFO] Copying user threads to image file\n");
    if ( ( iSourceFd = open( argv[5], O_RDONLY ) ) == -1 )
    {
     fprintf( stderr, "[ERROR] %s open fail\n", argv[ 5 ] );
     exit( -1 );
    }
    iSourceSize = CopyFile( iSourceFd, iTargetFd );
    close( iSourceFd );

    uthread_sector_count += AdjustInSectorSize( iTargetFd, iSourceSize );
    printf( "[INFO] %s size = [%d] and sector count = [%d]\n",
             argv[ 5 ], iSourceSize, uthread_sector_count );


    //--------------------------------------------------------------------------
    // 디스크 이미지에 커널 정보를 갱신
    //--------------------------------------------------------------------------
    printf( "[INFO] Start to write kernel information\n" );
    // 부트섹터의 5번째 바이트부터 커널에 대한 정보를 넣음
    WriteKernelInformation( iTargetFd, iKernel32SectorCount + iPageTableSize + iKernel64SectorCount + uthread_sector_count,
            iKernel32SectorCount, iPageTableSize, iKernel64SectorCount );
    printf( "[INFO] Image file create complete\n" );

    close( iTargetFd );
    return 0;
}

/**
 *  현재 위치부터 512바이트 배수 위치까지 맞추어 0x00으로 채움
*/
int AdjustInSectorSize( int iFd, int iSourceSize )
{
    int i;
    int iAdjustSizeToSector;
    char cCh;
    int iSectorCount;

    iAdjustSizeToSector = iSourceSize % BYTESOFSECTOR;
    cCh = 0x00;
    
    if( iAdjustSizeToSector != 0 )
    {
        iAdjustSizeToSector = 512 - iAdjustSizeToSector;
        printf( "[INFO] File size [%u] and fill [%u] byte\n", iSourceSize, 
            iAdjustSizeToSector );
        for( i = 0 ; i < iAdjustSizeToSector ; i++ )
        {
            write( iFd , &cCh , 1 );
        }
    }
    else
    {
        printf( "[INFO] File size is aligned 512 byte\n" );
    }
    
    // 섹터 수를 되돌려줌
    iSectorCount = ( iSourceSize + iAdjustSizeToSector ) / BYTESOFSECTOR;
    return iSectorCount;
}

/**
 *  부트 로더에 커널에 대한 정보를 삽입
*/
void WriteKernelInformation( int iTargetFd, int iTotalKernelSectorCount,
        int iKernel32SectorCount, int iPageTableCount, int kernel64_sector_count )
{
    unsigned short usData;
    long lPosition;

    // 파일의 시작에서 5바이트 떨어진 위치가 커널의 총 섹터 수 정보를 나타냄
    lPosition = lseek( iTargetFd, 0x5a, SEEK_SET );
    if( lPosition == -1 )
    {
        fprintf( stderr, "lseek fail. Return value = %ld, errno = %d, %d\n",
            lPosition, errno, SEEK_SET );
        exit( -1 );
    }

    // 부트 로더를 제외한 총 섹터 수 및 보호 모드 커널의 섹터 수 저장
    usData = ( unsigned short ) iTotalKernelSectorCount;
    write( iTargetFd, &usData, 2 );
    usData = ( unsigned short ) iKernel32SectorCount;
    write( iTargetFd, &usData, 2 );
    usData = ( unsigned short ) iPageTableCount;
    write( iTargetFd, &usData, 2 );
    usData = ( unsigned short ) kernel64_sector_count;
    write( iTargetFd, &usData, 2 );

    printf( "[INFO] Total sector count except boot loader [%d]\n",
        iTotalKernelSectorCount );
    printf( "[INFO] Total sector count of protected mode kernel [%d]\n",
        iKernel32SectorCount );
    printf( "[INFO] Total sector count of IA-32e pagetable [%d]\n",
        iPageTableCount );
    printf( "[INFO] Total sector count of IA-32e mode kernel [%d]\n",
        kernel64_sector_count );
    printf( "[INFO] Total sector count of user thread [%d]\n",
        iTotalKernelSectorCount - iKernel32SectorCount - iPageTableCount-kernel64_sector_count );
}


/**
 *  소스 파일(Source FD)의 내용을 목표 파일(Target FD)에 복사하고 그 크기를 되돌려줌
*/
int CopyFile( int iSourceFd, int iTargetFd )
{
    int iSourceFileSize;
    int iRead;
    int iWrite;
    char vcBuffer[ BYTESOFSECTOR ];

    iSourceFileSize = 0;
    while( 1 )
    {
        iRead   = read( iSourceFd, vcBuffer, sizeof( vcBuffer ) );
        iWrite  = write( iTargetFd, vcBuffer, iRead );

        if( iRead != iWrite )
        {
            fprintf( stderr, "[ERROR] iRead != iWrite.. \n" );
            exit(-1);
        }
        iSourceFileSize += iRead;
        
        if( iRead != sizeof( vcBuffer ) )
        {
            break;
        }
    }
    return iSourceFileSize;
} 
