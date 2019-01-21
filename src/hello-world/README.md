```sh
# command-line examples:

 $ K-hello-world                             # print logs and result to screen
 $ K-hello-world > my_result                 # print logs to screen and write result to file
 $ K-hello-world 2> my_log_file              # write logs to file and print result to screen
 $ K-hello-world > my_log_file 2>&1          # write logs and result to same file
 $ K-hello-world 2> my_log_file > my_result  # write logs and result to different files
 $ K-hello-world | cowsay                    # print verbose speaking cow notification to screen
 $ K-hello-world 2> /dev/null | cowsay       # print speaking cow notification to screen

# enjoy!
```
