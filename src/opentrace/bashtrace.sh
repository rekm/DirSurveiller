#!/bin/bash



# This Program is largely based on perf-utils software bundle
# created by Brandan Gregg
#
#  It is an amalgamation of his opensnoop
#  and execsnoop program
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#  (http://www.gnu.org/copyleft/gpl.html)
#

### default variables
tracing=/sys/kernel/debug/tracing
flock=/var/tmp/.ftrace-lock; wroteflock=0
opt_duration=0; duration=; opt_name=0; name=; opt_time=0; opt_reexec=0
opt_argc=0; argc=8; max_argc=16; ftext=
trap ':' INT QUIT TERM PIPE HUP	# sends execution to end tracing section

function warn {
	if ! eval "$@"; then
		echo >&2 "WARNING: command failed \"$@\""
	fi
}

function end {
	# disable tracing
	echo 2>/dev/null
	echo "Ending tracing..." 2>/dev/null
	cd $tracing
	warn "echo 0 > events/kprobes/$kname/enable"
	warn "echo 0 > events/kprobes/getnameprobe/enable"
	warn "echo 0 > events/syscalls/sys_exit_open/enable"
	warn "echo 0 > events/sched/sched_process_fork/enable"
	warn "echo -:$kname >> kprobe_events"
	warn "echo -:getnameprobe >> kprobe_events"
	warn "echo > trace"
	(( wroteflock )) && warn "rm $flock"
}

function die {
	echo >&2 "$@"
	exit 1
}

function edie {
	# die with a quiet end()
	echo >&2 "$@"
	exec >/dev/null 2>&1
	end
	exit 1
}

ftext=" opens and forks"
### select awk
(( opt_duration )) && use=mawk || use=gawk	# workaround for mawk fflush()
[[ -x /usr/bin/$use ]] && awk=$use || awk=awk

### check permissions
cd $tracing || die "ERROR: accessing tracing. Root user? Kernel has FTRACE?
    debugfs mounted? (mount -t debugfs debugfs /sys/kernel/debug)"

### ftrace lock
[[ -e $flock ]] && die "ERROR: ftrace may be in use by PID $(cat $flock) $flock"
echo $$ > $flock || die "ERROR: unable to write $flock."
wroteflock=1

### build probe
if [[ -x /usr/bin/getconf ]]; then
	bits=$(getconf LONG_BIT)
else
	bits=64
	[[ $(uname -m) == i* ]] && bits=32
fi
(( offset = bits / 8 ))
function makeprobe {
	func=$1
	kname=execsnoop_$func
	kprobe1="p:$kname $func"
	i=0
	while (( i < argc + 1 )); do
		# p:kname do_execve +0(+0(%si)):string +0(+8(%si)):string ...
		kprobe1="$kprobe1 +0(+$(( i * offset ))(%si)):string"
		(( i++ ))
	done
}

# try in this order: sys_execve, stub_execve, do_execve
makeprobe sys_execve

### setup and begin tracing
echo nop > current_tracer

# Set up probes
if ! echo $kprobe1 >> kprobe_events 2>/dev/null; then
	makeprobe stub_execve
	if ! echo $kprobe1 >> kprobe_events 2>/dev/null; then
	    makeprobe do_execve
	    if ! echo $kprobe1 >> kprobe_events 2>/dev/null; then
		    edie "ERROR: adding a kprobe for execve. Exiting."
        fi
	fi
fi

ver=$(uname -r)
if [[ "$ver" == 2.* || "$ver" == 3.[1-6].* ]]; then
	# rval is char *
	kprobe='r:getnameprobe getname +0($retval):string'
else
	# rval is struct filename *
	kprobe='r:getnameprobe getname +0(+0($retval)):string'
fi

if ! echo $kprobe >> kprobe_events; then
	edie "ERROR: adding a kprobe for getname(). Exiting."
fi

if ! echo 1 > events/kprobes/getnameprobe/enable; then
	edie "ERROR: enabling kprobe for getname(). Exiting."
fi
if ! echo 1 > events/syscalls/sys_exit_open/enable; then
	edie "ERROR: enabling open() exit tracepoint. Exiting."
fi


if ! echo 1 > events/kprobes/$kname/enable; then
	edie "ERROR: enabling kprobe for execve. Exiting."
fi
if ! echo 1 > events/sched/sched_process_fork/enable; then
	edie "ERROR: enabling sched:sched_process_fork tracepoint. Exiting."
fi
echo "Instrumenting $func"

#
# Determine output format. It may be one of the following (newest first):
#           TASK-PID   CPU#  ||||    TIMESTAMP  FUNCTION
#           TASK-PID    CPU#    TIMESTAMP  FUNCTION
# To differentiate between them, the number of header fields is counted,
# and an offset set, to skip the extra column when needed.
#
offset=$($awk 'BEGIN { o = 0; }
	$1 == "#" && $2 ~ /TASK/ && NF == 6 { o = 1; }
	$2 ~ /TASK/ { print o; exit }' trace)

### print trace buffer
warn "echo > trace"

cat -v trace_pipe | $awk -v o=$offset -v kname=$kname '
    #common fields
    $1 != "#" {
        # task name can contain dashes and numbers
        split($0, line, "-")
        sub(/^[ \t\r\n]+/, "", line[1])
        comm = line[1]
        sub(/ .*$/, "", line[2])
        opid = line[2]
        pid = line[2]
    }

	# do_sys_open()
	$1 != "#" && $(5+o) ~ /do_sys_open/ {
		#
		# eg: ... (do_sys_open+0xc3/0x220 <- getname) arg1="file1"
		#
		filename = $NF
		sub(/"$/, "", filename)
		sub(/.*"/, "", filename)
		lastfile[opid] = filename
	}

	# sys_open()
	$1 != "#" && $(4+o) == "sys_open" {
		filename = lastfile[opid]
		delete lastfile[opid]
		if (opt_file && filename !~ file)
			next
		rval = $NF
		# matched failed as beginning with 0xfffff
		if (opt_fail && rval !~ /0xfffff/)
			next
		if (rval ~ /0xfffff/)
			rval = -1

		if (opt_time) {
			time = $(3+o); sub(":", "", time)
			printf "%-16s ", time
		}
		printf "%-16.16s %-6s %4s %s\n", comm, opid, rval, filename
	}
    #execve output

	$1 != "#" && $(4+o) ~ /sched_process_fork/ {
		cpid=$0
		sub(/.* child_pid=/, "", cpid)
		sub(/ .*/, "", cpid)
		getppid[cpid] = pid
		delete seen[pid]
	}

	$1 != "#" && $(4+o) ~ kname {
		if (seen[pid])
			next
		#if (opt_name && comm !~ name)
		#	next

		#
		# examples:
		# ... arg1="/bin/echo" arg2="1" arg3="2" arg4="3" ...
		# ... arg1="sleep" arg2="2" arg3=(fault) arg4="" ...
		# ... arg1="" arg2=(fault) arg3="" arg4="" ...
		# the last example is uncommon, and may be a race.
		#
		if ($0 ~ /arg1=""/) {
			args = comm " [?]"
		} else {
			args=$0
			sub(/ arg[0-9]*=\(fault\).*/, "", args)
			sub(/.*arg1="/, "", args)
			gsub(/" arg[0-9]*="/, " ", args)
			sub(/"$/, "", args)
			if ($0 !~ /\(fault\)/)
				args = args " [...]"
		}

		#if (opt_time) {
		time = $(3+o); sub(":", "", time)
		printf "%-16s ", time
		#}
		printf "%6s %6d %s\n", pid, getppid[pid], args >> "/dev/stderr"
		#if (!opt_duration)
		fflush()
		#if (!opt_reexec) {
		seen[pid] = 1
		delete getppid[pid]
		#}
	}

	$0 ~ /LOST.*EVENT[S]/ { print "WARNING: " $0 > "/dev/stderr" }
'

# end tracing
end

