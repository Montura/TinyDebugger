# TinyDebugger


Writing a tiny debuuger.

Inspired by https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/


| Terminal commands  | Help |
| ------------- | ------------- |
| continue  | Continue debugee execution  |
| break     |  <table>  <thead>  <th>  Set break point at </th>  <th>Format</th>  </tr>  </thead>  <tbody>  <tr>  <td>Addres</td>  <td>0x555555554656</td>  </tr>  <tr>  <td>Function name</td>  <td>test</td>  </tr>  <tr>  <td>Source line</td>  <td>main.cpp:22</td>  </tr> </tbody>  </table>  |
| register |  <table>  <thead>  <th>  Apply op to register </th>  <th>Format</th>  </tr>  </thead>  <tbody>  <tr>  <td>read</td>  <td>rip</td>  </tr>  <tr>  <td>write</td>  <td>0x555555554656</td>  </tr> <tr>  <td>dump</td>  <td>print all registers to console</td>  </tr> </tbody>  </table>  | 
| memory |  <table>  <thead>  <th>  Apply op to memory </th>  <th>Format</th>  </tr>  </thead>  <tbody>  <tr>  <td>read</td>  <td>0x555555554656</td>  </tr>  <tr>  <td>write</td>  <td>addr value (0x555555554656 12)</td>  </tr> </tbody>  </table> |
| stepi  | Step in with one instruction |
| step  | Step in |
| next  | Step over |
| finish  | Step out |
| symbol  | Lookup symbol in sources (symbol name) |
| backtrace  | Print backtrace |
| vars  | Print local variables in function |
