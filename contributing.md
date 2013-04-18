# Contributing to Cherokee Webserver

We love to see your interest in contributing to Cherokee Webserver!

This guide covers all you need to know to make this a fluid experience.

## Table of contents

1 [Issues list](#issues-iist)  
1.1 [Writing a new issue](#writing-a-new-issue)  
1.1.1 [Finding a suitable title](#finding-a-suitable-title)  
1.2 [Issue labels](#issue-labels)  

## Issues list

### Writing new issues

So you found a bug, have a feature request, would like to see a functionality being enhanced or just have a simple question? Well -- that is just marvelous. Here is how you write a good ticket to make the process as easy as possible.

#### Finding a suitable title

Give your issue a caption that is both, **descriptive and precise**. If your title makes it possible to grasp the meaning of your request, you are helping the developers to easily label the issue and pick those out who are important.

Here are some good examples:
- *“Worker using 100% cpu with big files.”*
- *“Information Source ignoring Spawning Timeout.”*
- *“Content-Encoding header not implemented for static files already compressed.”*

And here are some not so good:
- *“504 Gateway error”*  
  This is too generic, how about: “*Random 504 gateway errors in reverse proxy mode.*”?
- *“error message running cherokee-admin”*  
  An itsy-bitsy bit better, but can still be improved: “*`cherokee-admin -u` throws: could not create socket.*”

But **don't get to technical** over the fact that we prefer precise titles. Remember, you already know the defect behind that issue, and even the problematic line of code, the developer might just not and needs to read the description to understand what exactly is the problem. Such information are appropriate for the descripton, not for the title.

A bad example:
- *“fdpoll-epoll.c:113: epoll_ctl(6, EPOLL_CTL_ADD, 3): 'File exists'”*

At the same time the opener provided a perfect line title just in the description.
- *“Cherokee isn’t aware of EPOLLs and fails when it’s already present.”*

### Comprehending issue labels
