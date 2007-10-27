#! /bin/bash
#
# Generate some basic versioning information which can be piped to a header.
#
# Copyright (c) 2006-2007 Luc Verhaegen <libv@skynet.be>

echo "/*"
echo " * Basic versioning gathered from the git repository."
echo " */"
echo ""
# this might not correspond with the final filename, but will be close enough.
echo "#ifndef HAVE_GIT_VERSION_H"
echo "#define HAVE_GIT_VERSION_H 1"
echo ""

git_tools=`which git-whatchanged`
if test "$git_tools" != ""; then
    if [ -e .git/index ]; then
        echo "/* This is a git repository */"
        echo "#define GIT_USED 1"
        echo ""

        # SHA-ID
        git_shaid=`git-whatchanged | head -n1 | sed s/^commit\ //`
        echo "/* Git SHA ID of last commit */"
        echo "#define GIT_SHAID \"${git_shaid:0:8}..\""
        echo ""

        # Branch -- use git-status instead of git-branch
        git_branch=`git-status | grep "# On branch" | sed s/#\ On\ branch\ //`
        if test "$git_branch" = ""; then
            git_branch="master"
        fi
        echo "/* Branch this tree is on */"
        echo "#define GIT_BRANCH \"$git_branch\""
        echo ""

        # Any uncommitted changes we should know about?
        git_uncommitted=`git-status | grep "Changed but not updated"`
        if test "$git_uncommitted" = ""; then
            git_uncommitted=`git-status | grep "Updated but not checked in"`
        fi

        if test "$git_uncommitted" != ""; then
            echo "/* Local changes might be breaking things */"
            echo "#define GIT_UNCOMMITTED 1"
        else
            echo "/* SHA-ID uniquely defines the state of this code */"
            echo "#undef GIT_UNCOMMITTED"
        fi
    else
        echo "/* This is not a git repository */"
        echo "#undef GIT_USED"
    fi
else
    echo "/* git is not installed */"
    echo "#undef GIT_USED"
fi

echo ""
echo "#endif /* HAVE_GIT_VERSION_H */"

