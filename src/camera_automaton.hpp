#ifndef __camera_automaton_hpp__
#define __camera_automaton_hpp__

#include <ioa/ioa.hpp>
#include <ioa/buffer.hpp>

#include <iostream>
#include <string>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <asm/types.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define REQ_BUFFER_COUNT 20

class camera_automaton :
  public ioa::automaton
{
private:
  struct buffer_t {
    void* start;
    size_t length;
  };

  const size_t m_page_size;
  int m_fd;
  v4l2_format m_fmt;
  v4l2_requestbuffers m_req;
  buffer_t* m_buffers;

private:
  static int xioctl (int fd,
		     int request,
		     void* arg) {
    int r;
    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
  }

public:
  camera_automaton (const std::string& path) :
    m_page_size (getpagesize ())
  {
    // Open the camera device.
    struct stat st;
    if (stat (path.c_str (), &st) == -1) {
      perror ("stat");
      exit (EXIT_FAILURE);
    }

    if (!S_ISCHR (st.st_mode)) {
      std::cerr << path << " is not a character device" << std::endl;
      exit (EXIT_FAILURE);
    }

    m_fd = open (path.c_str (), O_RDWR /* required */ | O_NONBLOCK, 0);
    if (m_fd == -1) {
      perror ("open");
      exit (EXIT_FAILURE);
    }

    // Initialize the device.
    v4l2_capability cap;
    v4l2_cropcap cropcap;
    v4l2_crop crop;

    CLEAR (cap);

    if (xioctl (m_fd, VIDIOC_QUERYCAP, &cap) == -1) {
      if (EINVAL == errno) {
	std::cerr << path << " is not a V4L2 device" << std::endl;
	exit (EXIT_FAILURE);
      }
      else {
	perror ("VIDIOC_QUERYCAP");
	exit (EXIT_FAILURE);
      }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
      std::cerr << path << " is not a video capture device" << std::endl;
      exit (EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
      std::cerr << path << " does not support streaming I/O" << std::endl;
      exit (EXIT_FAILURE);
    }

    // Select video input, video standard and tune here.

    CLEAR (cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (xioctl (m_fd, VIDIOC_CROPCAP, &cropcap) == 0) {
      crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      crop.c = cropcap.defrect; /* reset to default */
      
      if (xioctl (m_fd, VIDIOC_S_CROP, &crop)== -1) {
    	switch (errno) {
    	case EINVAL:
    	  /* Cropping not supported. */
    	  break;
    	default:
    	  /* Errors ignored. */
    	  break;
    	}
      }
    } else {
      /* Errors ignored. */
    }

    CLEAR (m_fmt);
    m_fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl (m_fd, VIDIOC_G_FMT, &m_fmt) == -1) {
      perror ("VIDIOC_G_FMT");
      exit (EXIT_FAILURE);
    }

    // std::cout << "fmt.type = " << fmt.type << std::endl;
    // std::cout << "fmt.fmt.pix.width = " << fmt.fmt.pix.width << std::endl;
    // std::cout << "fmt.fmt.pix.height = " << fmt.fmt.pix.height << std::endl;
    // std::cout << "fmt.fmt.pix.pixelformat = " << fmt.fmt.pix.pixelformat << std::endl;
    // std::cout << V4L2_PIX_FMT_YUV420 << std::endl;
    // std::cout << "fmt.fmt.pix.field = " << fmt.fmt.pix.field << std::endl;
    // std::cout << V4L2_FIELD_NONE << std::endl;
    // std::cout << "fmt.fmt.pix.bytesperline = " << fmt.fmt.pix.bytesperline << std::endl;
    // std::cout << "fmt.fmt.pix.sizeimage = " << fmt.fmt.pix.sizeimage << std::endl;
    // std::cout << "fmt.fmt.pix.colorspace = " << fmt.fmt.pix.colorspace << std::endl;

    // fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // fmt.fmt.pix.width       = 640;
    // fmt.fmt.pix.height      = 480;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
    // fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    // if (xioctl (m_fd, VIDIOC_S_FMT, &fmt) == -1) {
    //   perror ("xioctl");
    //   exit (EXIT_FAILURE);
    // }
  
    // /* Note VIDIOC_S_FMT may change width and height. */
    // assert (fmt.fmt.pix.width == 352);
    // assert (fmt.fmt.pix.height == 288);
    // assert (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG);

    //m_buffer_size = (m_fmt.fmt.pix.sizeimage + m_page_size - 1) & ~(m_page_size - 1);
    

    CLEAR (m_req);
    m_req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    m_req.memory              = V4L2_MEMORY_MMAP;
    m_req.count               = REQ_BUFFER_COUNT;
    
    if (xioctl (m_fd, VIDIOC_REQBUFS, &m_req) == -1) {
      if (EINVAL == errno) {
	std::cerr << path << " does not support mmap I/O" << std::endl;
        exit (EXIT_FAILURE);
      }
      else {
        perror ("VIDIOC_REQBUFS");
        exit (EXIT_FAILURE);
      }
    }
    
    m_buffers = new buffer_t[m_req.count];
    
    for (unsigned int i = 0; i < m_req.count; ++i) {
      v4l2_buffer buffer;
      CLEAR (buffer);
      buffer.type = m_req.type;
      buffer.memory = V4L2_MEMORY_MMAP;
      buffer.index = i;
      
      if (xioctl (m_fd, VIDIOC_QUERYBUF, &buffer) == -1) {
	perror ("VIDIOC_QUERYBUF");
	exit (EXIT_FAILURE);
      }

      m_buffers[i].length = buffer.length; /* remember for munmap() */

      m_buffers[i].start = mmap (NULL, buffer.length,
				 PROT_READ | PROT_WRITE, /* recommended */
				 MAP_SHARED,             /* recommended */
				 m_fd, buffer.m.offset);
      
      if (MAP_FAILED == m_buffers[i].start) {
	/* If you do not exit here you should unmap() and free()
	   the buffers mapped so far. */
	perror ("mmap");
	exit (EXIT_FAILURE);
      }
    }

    schedule ();
  }

  ~camera_automaton () {
    for (unsigned int i = 0; i < m_req.count; i++) {
      munmap (m_buffers[i].start, m_buffers[i].length);
    }

    delete[] m_buffers;
  }

private:
  void schedule () const {
    if (send_precondition ()) {
      ioa::schedule (&camera_automaton::send);
    }
  }

private:
  bool send_precondition () const {
    return false;  // Don't care if its bound.
  }

  ioa::const_shared_ptr<ioa::buffer> send_effect () {

  }

public:
  V_UP_OUTPUT (camera_automaton, send, ioa::const_shared_ptr<ioa::buffer>);
};

#endif
