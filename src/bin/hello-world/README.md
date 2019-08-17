```sh
# command-line examples:

 $ K-hello-world                             # print result and logs
 $ K-hello-world 2> /dev/null                # print result
 $ K-hello-world > /dev/null                 # print logs
 $ K-hello-world 2> my_logs                  # print result and write logs to file
 $ K-hello-world > my_result                 # print logs and write result to file
 $ K-hello-world > my_result 2>&1            # write logs and result to same file
 $ K-hello-world > my_result 2> my_logs      # write logs and result to different files
 $ K-hello-world | cowsay                    # print verbose speaking cow notification
 $ K-hello-world 2> /dev/null | cowsay       # print speaking cow notification:
 ______________________________
/ Hello, WORLD!                \
|                              |
\ pssst.. 1 BTC = 9999.99 EUR. /
 ------------------------------
        \   ^__^
         \  (oo)\_______
            (__)\       )\/\
                ||----w |
                ||     ||

# enjoy!
```
