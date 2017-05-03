#!/bin/bash
#
#
#   FlameGraph CLI using Linux's perf for profiling
#   FlameGraph git repo: https://github.com/brendangregg/FlameGraph
#
#   NOTE: perf may need tweaking to grant tracing privileges to regular users.
#         The kernel variable controlling this is "perf_event_paranoid"
#         Usually a value of '1' is enough for our purposes, e.g.
#
#         ~$ sudo sysctl kernel.perf_event_paranoid=1
#
#         or to make it resilient after reboot:
#
#         ~$ sudo echo "kernel.perf_event_paranoid=1" >> /etc/sysctl.d/perf.conf
#
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at docs/cddl1.txt or
# http://opensource.org/licenses/CDDL-1.0.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at docs/cddl1.txt.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# 30-May-2016	Carlos E. Budde	Created this.


if [ $# -lt 1 ]; then
	echo "Invocation: flamegraph <command> [<arg1> <arg2> ...]"
	exit 1
elif ! hash perf &>/dev/null; then
	# http://stackoverflow.com/a/677212
	echo "Error: 'perf' not found"
	echo "Please install any system package providing perf to continue"
	exit 1
else
	echo "About to profile '$@'"
	echo "Remember debugging symbols are needed, e.g. your program should've"
	echo "been compiled with the '-g' option"
	echo -n "Proceed [Y/n]? "
	shopt -s nocasematch  # case insensitive match
	read PROCEED
	if [[ "YES" =~ "$PROCEED" ]]; then
		echo "Profiling info will be stored in trace.{data,perf,folded,svg}"
		sleep 1
		SCRIPTDIR=$(dirname `readlink -f $0`)
	elif [[ "NO" =~ "$PROCEED" ]]; then
		echo 'Aborting'
		exit 0
	else
		echo "Invalid option '$PROCEED' -- Aborting"
		exit 1
	fi
fi

perf record -F 500 --call-graph dwarf -o trace.data -g $@
perf script -i trace.data > trace.perf
$SCRIPTDIR/stackcollapse-perf.pl trace.perf > trace.folded
$SCRIPTDIR/flamegraph.pl trace.folded > trace.svg

echo
echo "Profiling ended successfully: view 'trace.svg' with any web browser"
echo "Alternatively 'perf report -g -i trace.data' uses the ncurses interface"

unset -v SCRIPTDIR PROCEED
exit 0
