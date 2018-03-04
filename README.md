# Arch-Project-List
## How to Run (Project1)
* make
* ./sim.exe [-flags] [.dump] [> result_file.txt]
  * [-flags] = -[i,r,m,d] (For more info on the flags, please refer to CSE140_Project1.pdf)
  * [.dump] = any dump file with instructions encoded. sample.dump is given.
  * [> result_file.txt] = pipeline output (except DEBUG statement)
  * Ex: ./sim -d sample.dump > result.txt