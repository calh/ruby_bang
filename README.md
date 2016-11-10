# ruby_bang #
## A shebang wrapper to RVM for Linux ##

### The Impatient Guide ###

```
[me@mypooter ruby_bang]$ sudo make install
gcc -Wall -O3 -o ruby_bang ruby_bang.c
/usr/bin/install ruby_bang /bin/ruby_bang
```

----
#### `my_script.rb` ####
```ruby
#!/bin/ruby_bang 2.2.0 do ruby
puts RUBY_VERSION
```

```
$ chmod 755 myscript.rb
$ ./my_script.rb
2.2.0
```

## Introduction ##

* Have you been pulling your hair out, trying to get a shebang script to work with RVM?
* Do you want to use multiple scripts in a crontab with different rubies?
* Are you running Linux with wonky argument passing to shebang executables?
  * ( `#!/usr/bin/env rvm 2.2.0 do ruby` actually works on OSX )
  
Then this is the app for **YOU**!

### How it works ###

`ruby_bang` is a very simple wrapper to [RVM](https://rvm.io).  It first searches
a few common spots where RVM might be installed.  $PATH, $HOME/.rvm/bin, and /usr/local/rvm/bin/rvm.  

After it finds an RVM installation, it splits the text after the shebang executable 
into space delimited arguments.  `ruby_bang` then takes those arguments and calls `execve` to the RVM script.
Your ruby script's name (`ARGV[0]`) is passed to RVM as the last argument.

### Working with Bundler ###

Some scripts just need a little extra jazz with a bundle.  `ruby_bang` will automatically `bundle check`
and `bundle install` if the parameters after `do` have a `bundle exec` in them.

Here's an example:

#### `curl.rb` ####
```ruby:
#!/bin/ruby_bang 2.1.1 do bundle exec ruby
require "curb"
curl=Curl::Easy.new
curl.url = "http://google.com"
curl.perform
puts curl.body_str
```
#### `Gemfile` ####
```
source "https://rubygems.org"
gem 'curb'
```

```
$ ./curl.rb
Fetching gem metadata from https://rubygems.org/..
Resolving dependencies...
Installing curb (0.9.3)
Using bundler (1.5.3)
Your bundle is complete!
Use `bundle show [gemname]` to see where a bundled gem is installed.
<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
<H1>301 Moved</H1>
The document has moved
<A HREF="http://www.google.com/">here</A>.
</BODY></HTML>
```

The bundle check output is hidden, and the next time you run the script with a correct bundle:

```
$ ./curl.rb
<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
...
```

### Why the name needs to begin with `ruby` ###

In various attempts at trying tricks to use bash and RVM as a shebang interpreter, I found that the ruby parser kicks out
this error:

```
ruby: no Ruby script found in input (LoadError)
```

If the shebang line does not contain the word `ruby` in the beginning of the executable name, the ruby parser seems to 
think it's not a ruby script.  Here's one attempt I tried:

```
#!/bin/bash
''''exec rvm 2.1.1 do ruby -i -- "$0" ${1+"$@"} # '''
puts RUBY_VERSION
```

And, another proof of concept:

```
#!/bin/judge_fudge
puts RUBY_VERSION
```

```
$ ruby fudge.rb
ruby: no Ruby script found in input (LoadError)
```

### You can do everything RVM does ###

Here are some more advanced examples:

```
#!/bin/ruby_bang 1.9.3,2.2.0 do ruby
puts RUBY_VERSION
```
```
$ ./myscript.rb
1.9.3
2.2.0
```

#### Gemset Example ####

```
$ rvm use ruby-2.1.1
$ rvm gemset create mygems
$ gem install curb
```

```
#!/bin/ruby_bang 2.1.1@mygems do ruby
require "curb"
curl=Curl::Easy.new
...
```

### Arguments work as expected ###

```
#!/bin/ruby_bang 1.9.3 do ruby
puts ARGV.inspect
```

```
$ ./argv.rb one two three four
["one", "two", "three", "four"]
```
