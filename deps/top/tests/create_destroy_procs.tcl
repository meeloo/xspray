
for {set runs 0} {$runs < 200} {incr runs} {
	set procs [list]

	for {set i 0} {$i < 50} {incr i} {
		lappend procs [exec /bin/ls &]
		lappend procs [exec [info nameofexecutable] ./create_destroy_child.tcl &]
		#Pick one at random to kill.
		set randi [expr {int(rand() * [llength $procs])}]
		set randproc [lindex $procs $i]
		set procs [lreplace $procs $i $i]
		catch {exec kill $randproc}
	}

	foreach p $procs {
		catch {exec kill $p}
	}
}
