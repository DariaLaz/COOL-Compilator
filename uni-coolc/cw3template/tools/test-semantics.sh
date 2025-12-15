script_dir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd -- "${script_dir}/.." && pwd)"

tool_name="semantics"

tests_dir="${project_root}/tests/${tool_name}"
bin_dir="${project_root}/build"
temp_dir="${project_root}/temp"

mkdir -p "${temp_dir}"

# first argument: verbose or input
verbose=""
input=""
silent=""

for arg in "$@"; do
    case "$arg" in
        verbose)
            verbose=true
            ;;
        -s)
            silent=true
            ;;
        *)
            input="$arg"
            ;;
    esac
done

run_diff() {
    local out_path="$1"
    local sol_path="$2"

    if head -1 "${out_path}" | grep -q "syntax error"; then
        diff <(head -1 "${out_path}") <(head -1 "${sol_path}")
    else
        diff "${out_path}" "${sol_path}"
    fi
}

run_test() {
    local input="$1"
    local testfile
    local testname
    local in_path out_path sol_path

    testfile="$(basename "${input}")"
    testname="${testfile%.in}"

    in_path="${tests_dir}/${testname}.in"
    out_path="${tests_dir}/${testname}.out"
    sol_path="${temp_dir}/${testname}.sol"

    total_tests=$((total_tests + 1))

    "${bin_dir}/${tool_name}" "${in_path}" > "${sol_path}"

    if ! diff_output=$(run_diff "${out_path}" "${sol_path}"); then
        echo "Test ${testname} FAILED"

        if [[ -z "$silent" ]]; then
            echo "=============================="
            echo "-------- INPUT --------"
            cat "${in_path}"
            echo
            echo "------ YOUR OUTPUT ------"
            cat "${sol_path}"
            echo
            echo "---- EXPECTED OUTPUT ----"
            cat "${out_path}"
            echo
            echo "--------- DIFF ---------"
            echo "${diff_output}"
            echo "=============================="
        fi
    else
        echo "Test ${testname} PASSED"
        passed_tests=$((passed_tests + 1))
    fi
}


# === Counters ===
total_tests=0
passed_tests=0
failed_tests=0

if [ -z "$input" ]; then
    for input in "${tests_dir}"/*.in; do
        run_test "${input}"
    done
else
    if [ -e "$input" ]; then
        run_test "${input}"
    else
        matches=( "${tests_dir}/${input}"*.in )
        if [ -e "${matches[0]}" ]; then
            for file in "${matches[@]}"; do
                run_test "${file}"
            done
        else
            echo "Error: '${input}' is not a valid file name or prefix"
            exit 1
        fi
    fi
fi

echo
if [ "$failed_tests" -eq 0 ]; then
    echo "✅ ALL TESTS PASSED (${passed_tests}/${total_tests})"
else
    echo "❌ ${failed_tests} TESTS FAILED (${passed_tests}/${total_tests} PASSED)"
fi
