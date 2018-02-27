# Callgrind Demo
Copied this code over from my OS-projects/ProgrammingAssignment3. Luckily it had a memory leak so we can demo Valgrind's memory check and Callgrind. This is Banker's algorithm, which is essentially a way of preventing deadlocks between threads by refusing to grant requests for resources that put the system in a state that could lead to a deadlock.

Details of the algorithm do not need to be covered, but the program accepts three integers as parameters for the number of resources each process needs to complete its work. Then, three processes (threads) are launched and they randomly request resources bounded by the arguments you provided. Everytime a request is made, Banker's algo runs to see if the request can be granted safely. Otherwise the requesting thread has to wait and try again later.

## Steps for this Demo
### Valgrind
- Add command line arguments if not populated (example: 5 5 5)
- Run Analyze > Valgrind Memory Analyzer
- There should be a memory leak on line 63, go to the Valgrind output and show they can see where the leak is allocated by clicking on the link in the loss record
- Tell them I should have freed the memory for the argument list after the threads were joined (after the loop on 69)

### Callgrind
- On the left side of the QtC window, change the profile from Debug to Release (button looks like a little computer)
- Run Analyze > Valgrind Function Profiler
- Look at the output pane, there are a ton of function calls and they are sorted by the number of instruction reads from each function (essentially the number of times an instruction was fetched while that function was running)
- The first two functions that are mine are `customer_process` and `release_resources`
- You can double click a function to get more information on what was called inside of the function
- Scroll to one of the two functions above and show them the code there
- Talk about Amdahl's Law - the speed up of a execution of a task when an improvement is made to any one part is limited by the ratio of time that one part has to the total execution time of that task (e.g. speeding up a function that is used a lot is going to have more benefits than speeding up one that isn't used a lot)
