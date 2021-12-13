_maple_complete()
{
    local cur_word prev_word type_list

    cur_word="${COMP_WORDS[COMP_CWORD]}"
    prev_word="${COMP_WORDS[COMP_CWORD-1]}"

    # generate a list of options it supports
    type_list=`maple --help`

    # Only perform completion by "--help parsing" if the current word starts with a dash ('-'),
    # meaning that the user is trying to complete an option.
    # Else, show input files as the options.
    # COMPREPLY is the array of possible completions, generated with
    # the compgen builtin.
    if [[ ${cur_word} == -* ]] ; then
	# Parse --help output to show options
        COMPREPLY=( $(compgen -W "${type_list}" -- ${cur_word}) $(compgen -f -- ${cur_word}) )
    else
	# Input files
	COMPREPLY=( $(compgen -f -- ${cur_word}) )
    fi
    return 0
}

# Register _maple_complete to provide completion for the maple driver
complete -F _maple_complete maple
