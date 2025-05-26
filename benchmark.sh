{
	echo "===== $(date) ====="
	wrk -t8 -c600 -d10s http://localhost:8000
	echo
} >> results.txt

