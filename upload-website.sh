#!/bin/sh

set -ev

fac

rsync -Lv web/* science.oregonstate.edu:public_html/fac
