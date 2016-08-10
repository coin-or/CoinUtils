#!/bin/bash

# Author: Ted Ralphs (ted@lehigh.edu)
# Copyright 2016, Ted Ralphs
# Released Under the Eclipse Public License 
#
# TODO
# - fix dependency-tracking or remove it from configure
# - consider using pushd/popd instead of cd somewhere/cd ..
# - look at TODO and FIXME below

# TODO consider using these options for safety
#set -u
#set -o pipefail

# script debugging
#set -x
#PS4='${LINENO}:${PWD}: '

function help {
    echo "Usage: get.dependencies.sh <command> --option1 --option2"
    echo
    echo "Commands:"
    echo
    echo "  fetch: Checkout source code for all dependencies"
    echo "    options: --svn (checkout from SVN)"
    echo "             --git (checkout from git)"
    echo "             --skip='proj1 proj2' skip listed projects"
    echo "             --no-third-party don't download third party source (getter-scripts)"
    echo
    echo "  build: Configure, build, test (optional), and pre-install all projects"
    echo "    options: --xxx=yyy (will be passed through to configure)"
    echo "             --monolithic do 'old style' monolithic build"
    echo "             --threads=n build in parallel with 'n' threads"
    echo "             --build-dir=\dir\to\build\in do a VPATH build (default: $PWD/build)"
    echo "             --test run unit test of main project before install"
    echo "             --test-all run unit tests of all projects before install"
    echo "             --verbosity=i set verbosity level"
    echo "             --reconfigure re-run configure"
    echo
    echo "  install: Install all projects in location specified by prefix"
    echo "    options: --prefix=\dir\to\install (where to install, default: $PWD)"
    echo
    echo "  uninstall: Uninstall all projects"
    echo
    echo "General options:"
    echo "  --debug: Turn on debugging output"
    echo 
}

function print_action {
    echo
    echo "##################################################"
    echo "### $1 "
    echo "##################################################"
    echo
}

function invoke_make {
    if [ $1 = 0 ]; then
        $MAKE -j $threads $2 >& /dev/null
    elif [ $1 = 1 ]; then
        $MAKE -j $threads $2 > /dev/null
    else
        $MAKE -j $threads $2
    fi
}

# Parse arguments
function parse_args {
    for arg in "$@"
    do
        case $arg in
            *=*)
                option=`expr "x$arg" : 'x\(.*\)=[^=]*'`
                option_arg=`expr "x$arg" : 'x[^=]*=\(.*\)'`
                case $option in
                    --prefix)
                        if [ "x$option_arg" != x ]; then
                            case $option_arg in
                                [\\/$]* | ?:[\\/]* | NONE | '' )
                                    prefix=$option_arg
                                    ;;
                                *)  
                                    echo "Prefix path must be absolute."
                                    exit 3
                                    ;;
                            esac
                        else
                            echo "No path provided for --prefix"
                            exit 3
                        fi
                        ;;
                    --build-dir)
                        if [ "x$option_arg" != x ]; then
                            case $option_arg in
                                [\\/$]* | ?:[\\/]* | NONE | '' )
                                    build_dir=$option_arg
                                    ;;
                                *)
                                    build_dir=$PWD/$option_arg
                                    ;;
                            esac
                        else
                            echo "No path provided for --build-dir"
                            exit 3
                        fi
                        ;;
                    --threads)  # FIXME these are actually not threads, but parallel processes (--jobs in makefile-speak)
                        if [ "x$option_arg" != x ]; then
                            threads=$option_arg
                        else
                            echo "No thread number specified for --threads"
                            exit 3
                        fi
                        ;;
                    --verbosity)
                        if [ "x$option_arg" != x ]; then
                            verbosity=$option_arg
                        else
                            echo "No verbosity specified for --verbosity"
                            exit 3
                        fi
                        ;;
                    DESTDIR)
                        echo "DESTDIR installation not supported"
                        exit 3
                        ;;
                    --skip)
                        if [ "x$option_arg" != x ]; then
                            coin_skip_projects=$option_arg
                        fi
                        ;;
                    *)
                        configure_options+="$arg "
                        ;;            
                esac
                ;;
            --sparse)
                sparse=true
                ;;
            --svn)
                svn=true
                ;;
            --git)
                svn=false
                ;;
            --debug)
                set -x
                ;;
            --monolithic)
                monolithic=true
                ;;
            --reconfigure)
                reconfigure=true
                ;;
            --test)
                run_test=true
                ;;
            --test-all)
                run_all_tests=true
                ;;
            --no-third-party)
                get_third_party=false
                ;;
            --*)
                configure_options+="$arg "
                ;;
            fetch)
                num_actions+=1
                fetch=true
                ;;
            build)
                num_actions+=1
                build=true
                ;;
            install)
                num_actions+=1
                install=true
                ;;
            uninstall)
                num_actions+=1
                uninstall=true
                ;;
        esac
    done
}

function fetch {

    # This changes the default separator used in for loops to carriage return.
    # We need this later.
    IFS=$'\n'

    # Keep track of the subdirectories in which we need to build later.
    subdirs=

    # Build list of sources for dependencies
    deps=`cat Dependencies | tr '\t' ' ' | tr -s ' '| cut -d ' ' -f 2-`

    for url in $deps
    do
        if [ `echo $url | cut -d '/' -f 3` != "projects.coin-or.org" ]; then
            # If this is a URL of something other than a COIN-OR project on
            # SVN, then we assume it's a git project
            git_url=`echo $url | tr '\t' ' ' | tr -s ' '| cut -d ' ' -f 1`
            branch=`echo $url | tr '\t' ' ' | tr -s ' '| cut -d ' ' -f 2`
            dir=`echo $git_url | cut -d '/' -f 5`
            proj=`echo $git_url | cut -d "/" -f 5`
            # Check whether we're supposed to skip this project
            skip_proj=false
            for i in $coin_skip_projects
            do
                if [ $proj = $i ]; then
                    skip_proj=true
                fi
            done
            if [ $skip_proj = "false" ]; then
                print_action "Clone $git_url branch $branch"
                if [ ! -e $dir ]; then
                    git clone --branch=$branch $git_url
                else
                    cd $dir
                    git pull origin $branch
                    cd -
                fi
                subdirs+="$dir "
            else
                echo "Skipping $proj..."
            fi
        elif [ $svn = "true" ]; then
            # Here, we are supposed to check out from SVN
            svn_repo=`echo $url | cut -d '/' -f 5`
            if [ $svn_repo = "BuildTools" ]; then
                if [ `echo $url | cut -d '/' -f 6` = 'ThirdParty' ]; then
                    tp_proj=`echo $url | cut -d '/' -f 7`
                    if [ `echo $url | cut -d '/' -f 8` = trunk ]; then
                        version=trunk
                    else
                        version=`echo $url | cut -d '/' -f 9`
                    fi
                    skip_proj=false
                    for i in $coin_skip_projects
                    do
                        if [ $tp_proj = $i ]; then
                            skip_proj=true
                        fi
                    done
                    if [ $skip_proj = "false" ]; then
                        mkdir -p ThirdParty
                        print_action "Checking out ThirdParty/$tp_proj"
                        svn co --non-interactive --trust-server-cert $url \
                            ThirdParty/$tp_proj
                    else
                        echo "Skipping $tp_proj..."
                    fi
                    if [ $get_third_party = "true" ] &&
                       [ -e ThirdParty/$tp_proj/get.$tp_proj ]; then
                        cd ThirdParty/$tp_proj
                        ./get.$tp_proj
                        cd -
                        subdirs+="ThirdParty/$tp_proj "
                    else
                        echo "Not downloading source for $tp_proj..."
                    fi
                fi
            else
                if [ $svn_repo = "CHiPPS" ]; then
                    proj=`echo $url | cut -d '/' -f 6`
                    if [ `echo $url | cut -d '/' -f 7` = trunk ]; then
                        version=trunk
                    else
                        version=`echo $url | cut -d '/' -f 8`
                    fi
                elif [ $svn_repo = "Data" ]; then
                    proj=`echo $url | cut -d '/' -f 5-6`
                    if [ `echo $url | cut -d '/' -f 7` = trunk ]; then
                        version=trunk
                    else
                        version=`echo $url | cut -d '/' -f 8`
                    fi
                else
                    proj=`echo $url | cut -d '/' -f 5`
                    if [ `echo $url | cut -d '/' -f 6` = trunk ]; then
                        version=trunk
                    else
                        version=`echo $url | cut -d '/' -f 7`
                    fi
                fi
                skip_proj=false
                for i in $coin_skip_projects
                do
                    if [ $proj = $i ]; then
                        skip_proj=true
                    fi
                done
                if [ $skip_proj = "false" ]; then
                    print_action "Checking out $proj"
                    svn co --non-interactive --trust-server-cert $url $proj
                    subdirs+="$proj "
                else
                    echo "Skipping $proj..."
                fi
            fi
        else
            # Otherwise, convert SVN URL to a Github one and check out with git
            svn_repo=`echo $url | cut -d '/' -f 5`
            if [ $svn_repo = 'Data' ]; then
                data_proj=`echo $url | cut -d '/' -f 6`
                print_action "Checking out Data/$data_proj"
                svn co $url Data/$data_proj
                subdirs+="Data/$data_proj "
            elif [ $svn_repo = 'BuildTools' ]; then
                if [ `echo $url | cut -d '/' -f 6` = "ThirdParty" ]; then
                    tp_proj=`echo $url | cut -d '/' -f 7`
                    proj=ThirdParty-$tp_proj
                    mkdir -p ThirdParty
                    if [ `echo $url | cut -d '/' -f 8` = "trunk" ]; then
                        branch=master
                        version=trunk
                    else
                        branch=`echo $url | cut -d '/' -f 8-9`
                        version=`echo $url | cut -d '/' -f 9`
                    fi
                    skip_proj=false
                    for i in $coin_skip_projects
                    do
                        if [ $tp_proj = $i ]; then
                            skip_proj=true
                        fi
                    done
                    if [ $skip_proj = "false" ]; then
                        print_action "Getting ThirdParty/$tp_proj branch $branch"
                        if [ ! -e ThirdParty/$tp_proj ]; then
                            git clone --branch=$branch \
                                https://github.com/coin-or-tools/$proj \
                                ThirdParty/$tp_proj
                            if [ $get_third_party = "true" ] && \
                                   [ -e ThirdParty/$tp_proj/get.$tp_proj ]; then
                                cd ThirdParty/$tp_proj
                                ./get.$tp_proj
                                cd -
                                subdirs+="ThirdParty/$tp_proj "
                            fi
                        else
                            cd ThirdParty/$tp_proj
                            git pull origin $branch
                            if [ $get_third_party = "true" ] && \
                                   [ -e get.$tp_proj ]; then
                                ./get.$tp_proj
                                subdirs+="ThirdParty/$tp_proj "
                            fi
                            cd -
                        fi
                    else
                        echo "Skipping $tp_proj..."
                    fi
                fi
            else
                if [ $svn_repo = "CHiPPS" ]; then
                    git_repo=CHiPPS-`echo $url | cut -d '/' -f 6`
                    proj=`echo $url | cut -d '/' -f 6`
                    if [ `echo $url | cut -d '/' -f 7` = 'trunk' ]; then
                        branch=master
                        version=trunk
                    else
                        branch=`echo $url | cut -d '/' -f 7-8`
                        version=`echo $url | cut -d '/' -f 8`
                    fi
                else
                    git_repo=`echo $url | cut -d '/' -f 5`
                    proj=`echo $url | cut -d '/' -f 5`
                    if [ `echo $url | cut -d '/' -f 6` = 'trunk' ]; then
                        branch=master
                        version=trunk
                    else
                        branch=`echo $url | cut -d '/' -f 6-7`
                        version=`echo $url | cut -d '/' -f 7`
                    fi
                fi
                skip_proj=false
                for i in $coin_skip_projects
                do
                    if [ $proj = $i ]; then
                        skip_proj=true
                    fi
                done
                if [ $skip_proj = "false" ]; then
                    print_action "Getting $git_repo branch $branch"
                    if [ sparse = "true" ]; then
                        mkdir $proj
                        cd $proj
                        git init
                        git remote add origin \
                            https://github.com/coin-or/$git_repo 
                        git config core.sparsecheckout true
                        echo $proj/ >> .git/info/sparse-checkout
                        git fetch
                        git checkout $branch
                        cd ..
                    else
                        if [ ! -e $proj ]; then
                            git clone --branch=$branch \
                                https://github.com/coin-or/$git_repo $proj
                        else
                            cd $proj
                            git pull origin $branch
                            cd -
                        fi
                    fi
                    subdirs+="$proj/$proj "
                else
                    echo "Skipping $proj..."
                fi
            fi
        fi
    done
    echo $subdirs > .subdirs
    unset IFS
}

function build {
    if [ $monolithic = "false" ]; then
        if [ ! -e ".subdirs" ]; then
            echo "No .subdirs file. Please fetch first"
        else
            mkdir -p $build_dir
            cp .subdirs $build_dir/coin_subdirs.txt
        fi
        for dir in `cat .subdirs`
        do
            if [ $build_dir != $PWD ]; then
                proj_dir=`echo $dir | cut -d '/' -f 1`
                if [ $proj_dir = "Data" ] || [ $proj_dir = "ThirdParty" ]; then
                    proj_dir=$dir
                fi
                mkdir -p $build_dir/$proj_dir
                cd $build_dir/$proj_dir
            else
                cd $dir
            fi
            if [ ! -e config.status ] || [ $reconfigure = "true" ]; then
                if [ $reconfigure = "true" ]; then
                    print_action "Reconfiguring $proj_dir"
                else
                    print_action "Configuring $proj_dir"
                fi
                if [ $verbosity != 0 ]; then
                    $root_dir/$dir/configure --disable-dependency-tracking \
                        --prefix=$1 $configure_options
                else
                    $root_dir/$dir/configure --disable-dependency-tracking \
                        --prefix=$1 $configure_options > /dev/null
                fi
            fi
            print_action "Building $proj_dir"
            invoke_make $verbosity
            if [ $run_all_tests = "true" ]; then
                print_action "Running $proj_dir unit test"
                invoke_make "false" test
            fi
            if [ $1 = $build_dir ]; then
                print_action "Pre-installing $proj_dir"
            else
                print_action "Installing $proj_dir"
            fi
            invoke_make $verbosity install
            cd $root_dir
        done
        if [ -e $main_proj ]; then
            if [ build_dir != $PWD ]; then
                mkdir -p $build_dir/$main_proj
                cd $build_dir/$main_proj
            else
                cd $main_proj
            fi
        else
            cd $build_dir
        fi
        if [ ! -e config.status ] || [ $reconfigure = "true" ]; then 
            if [ $reconfigure = "true" ]; then
                print_action "Reconfiguring $main_proj"
            else
                print_action "Configuring $main_proj"
            fi
            # First, check whether this is a "rootless" project
            if [ -e $root_dir/$main_proj/configure ]; then
                root_config=$root_dir/$main_proj/configure
            else
                root_config=$root_dir/configure
            fi
            # Now, do the actual configuration
            if [ $verbosity != 0 ]; then
                $root_config --disable-dependency-tracking \
                        --prefix=$1 $configure_options
            else
                $root_config --disable-dependency-tracking \
                        --prefix=$1 $configure_options > /dev/null
            fi
        fi
        print_action "Building $main_proj"
        invoke_make $verbosity
        if [ $run_test = "true" ]; then
            print_action "Running $main_proj unit test"
            invoke_make "false" test
        fi
        if [ $1 = $build_dir ]; then
            print_action "Pre-installing $main_proj"
        else
            print_action "Installing $main_proj"
        fi
        invoke_make $verbosity install
        cd $root_dir
    else
        if [ build_dir != $PWD ]; then
            mkdir -p $build_dir
            cd $build_dir
        fi
        if [ ! -e config.status ]; then
            print_action "Configuring"
        else
            if [ $reconfigure = true ]; then
                print_action "Reconfiguring"
            fi
        fi
        if [ $verbosity != 0 ]; then
            $root_dir/configure --disable-dependency-tracking \
                                --prefix=$1 $configure_options
        else
            $root_dir/configure --disable-dependency-tracking \
                                --prefix=$1 $configure_options > /dev/null
        fi
        if [ $run_all_tests = "true"]; then
            echo "Warning: Can't run all tests with a monolithic build."
            echo "Disabling setting"
            run_test=true
        fi
        print_action "Building"
        invoke_make $verbosity
        if [ $run_test = "true" ]; then 
            print_action "Running unit test"
            invoke_make "false" test
        fi
        invoke_make $verbosity install
        cd $root_dir
    fi
}

function install {
    if [ prefix != $build_dir ]; then
        print_action "Reconfiguring projects and doing final install"
        reconfigure=true
        build $prefix
    fi
}

function uninstall {
    if [ $monolithic = "false" ]; then
        if [ ! -e ".subdirs" ]; then
            echo "No .subdirs file. Please fetch first"
        fi
        subdirs=(`cat .subdirs`)
        # We have to uninstall in reverse order
        for ((dir=${#subdirs[@]}-1; i>=0; i--))
        do
            if [ build_dir != $PWD ]; then
                proj_dir=`echo $dir | cut -d '/' -f 1`
                if [ $proj_dir = "Data" ] || [ $proj_dir = "ThirdParty" ]; then
                    proj_dir=$dir
                fi
                cd $build_dir/$proj_dir
            else
                cd $dir
            fi
            print_action "Uninstalling $proj_dir"
            invoke_make $verbosity uninstall
            cd $root_dir
        done
        if [ -e $main_proj ]; then
            if [ build_dir != $PWD ]; then
                mkdir -p $build_dir/$main_proj
                cd $build_dir/$main_proj
            else
                cd $main_proj
            fi
        fi
        print_action "Uninstalling $main_proj"
        invoke_make $verbosity uninstall
        cd $root_dir
    else
        if [ build_dir != $PWD ]; then
            cd $build_dir
        fi
        print_action "Uninstalling"
        invoke_make $verbosity uninstall
        cd $root_dir
    fi
}
    
# Exit when command fails
set -e

# Set defaults
root_dir=$PWD
declare -i num_actions
num_actions=0
sparse=false
prefix=
coin_skip_projects=
svn=true
fetch=false
build=false
install=false
uninstall=false
run_test=false
run_all_tests=false
configure_options=
monolithic=false
threads=1
build_dir=$PWD/build
reconfigure=false
get_third_party=true
verbosity=2
MAKE=make

echo "Welcome to the COIN-OR fetch and build utility"
echo 
echo "For help, run script without arguments."
echo 

if [ -e configure.ac ]; then
    main_proj=`fgrep AC_INIT configure.ac | cut -d '[' -f 2 | cut -d ']' -f 1`
else
    echo "Unable to find root configure script."
    echo "Please run script in root directory of checkout."
    exit 2
fi

parse_args "$@"

if [ x"$prefix" != x ] && [ install = "false" ]; then
    echo "Prefix should only be specified at install"
    exit 3
fi
if [ x"$prefix" = x ]; then
    prefix=$build_dir
fi

if [ -e $build_dir/.config ]; then
    echo "Previous configuration options found."
    if [ x"$configure_options" != x ]; then
        echo "Options cannot be changed after initial configuration."
        echo "To build with a new configuration:"
        echo "   1. Specify new build directory"
        echo "   2. Delete $build_dir/.config and"
        echo "      re-run with --reconfigure (not recommended)"
        exit 3
    fi
    configure_options=`cat $build_dir/.config`
else
    if [ x"$configure_options" != x ] && [ build = "false" ]; then
        echo "Configuration options should be specified with build command"
        exit 3
    fi
    echo "Caching configuration options..."
    mkdir -p $build_dir
    echo "$configure_options" > $build_dir/.config
fi

# Help
if [ $num_actions == 0 ]; then
    help
fi

# Get sources
if [ $fetch = "true" ]; then
    fetch
fi

# Build (and possibly test) the code
if [ $build = "true" ]; then
    build $build_dir
fi

# Install code
if [ $install = "true" ]; then
    install
fi

# Uninstall code
if [ $uninstall = "true" ]; then
    uninstall
fi
