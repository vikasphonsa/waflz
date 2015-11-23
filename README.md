waflz
=====

**waflz** is a modsecurity implementation only supporting restricted set of operations.

**waflz** uses protocall buffers for its internal representation of modsecurity rules.
The rules themselves can be expressed in one of three formats:
* Modsecurity Rule format 
* Protocol Buffers (Binary format)
* json

The **waflz_dump** utility can convert between the three formats.

## waflz dump examples

#### Help
```bash
~>waflz_dump --help
Usage: waflz_dump [options]
Options are:
  -h, --help           Display this help and exit.
  -v, --version        Display the version number and exit.
  
Input Options: (type defaults to Modsecurity)
  -i, --input          File / Directory to read from (REQUIRED).
  -M, --input_modsec   Input Type Modsecurity rules (file or directory)
  -J, --input_json     Input Type Json
  -P, --input_pbuf     Input Type Binary Protobuf (Default).
  
Output Options: (type defaults to Json)
  -o, --output         File to write to.
  -j, --json           Output Type Json.
  -p, --pbuf           Output Type Binary Protobuf.
  -m, --modsec         Output Type ModSecurity format.
  
Filter Options: 
  -f, --filter         Filter.
  -F, --file_filter    Filter from file.
  
Print Options:
  -r, --verbose        Verbose logging
  -c, --color          Color
  -s, --status         Status -show unhandled modsecurity info
  
Debug Options:
  -G, --gprofile       Google profiler output file
```

#### Modsecurity to Json
```bash
~>waflz_dump -i data/modsecurity_shellshock_1.conf -j | json_pp
{
   "sec_rule" : [
      {
         "operator" : {
            "value" : "() {",
            "type" : "CONTAINS",
            "is_regex" : false
         },
         "variable" : [
            {
               "is_count" : false,
               "type" : "REQUEST_HEADERS",
               "match" : [
                  {
                     "is_negated" : false,
                     "is_regex" : false
                  }
               ]
            },
            {
               "type" : "REQUEST_LINE",
               "is_count" : false,
               "match" : [
                  {
                     "is_regex" : false,
                     "is_negated" : false
                  }
               ]
            },
            {
               "type" : "REQUEST_BODY",
               "is_count" : false,
               "match" : [
                  {
                     "is_regex" : false,
                     "is_negated" : false
                  }
               ]
            },
            {
               "match" : [
                  {
                     "is_regex" : false,
                     "is_negated" : false
                  }
               ],
               "is_count" : false,
               "type" : "REQUEST_HEADERS_NAMES"
            }
         ],
         "action" : {
            "msg" : "Bash shellshock attack detected",
            "ver" : "EC/1.0.0",
            "rev" : "1",
            "tag" : [
               "CVE-2014-6271"
            ],
            "maturity" : "1",
            "t" : [
               "NONE",
               "URLDECODEUNI",
               "UTF8TOUNICODE"
            ],
            "file" : "modsecurity_shellshock_1.conf",
            "phase" : 2,
            "action_type" : "BLOCK",
            "accuracy" : "8",
            "id" : "431000"
         },
         "order" : 0,
         "hidden" : false
      }
   ]
}
```

#### Modsecurity to Protobuf
```bash
~>waflz_dump -i data/modsecurity_shellshock_1.conf -p | xxd
0000000: c2bb 01ca 01c2 3e0a 0816 1204 5000 5800  ......>.....P.X.
0000010: 1800 c23e 0a08 1812 0450 0058 0018 00c2  ...>.....P.X....
0000020: 3e0a 0812 1204 5000 5800 1800 c23e 0a08  >.....P.X....>..
0000030: 1712 0450 0058 0018 00ca 3e0a 0802 1204  ...P.X....>.....
0000040: 2829 207b 1800 d23e 7e0a 0634 3331 3030  () {...>~..43100
0000050: 3012 1f42 6173 6820 7368 656c 6c73 686f  0..Bash shellsho
0000060: 636b 2061 7474 6163 6b20 6465 7465 6374  ck attack detect
0000070: 6564 5002 a206 0138 aa06 0131 b006 02ba  edP....8...1....
0000080: 0601 31c2 0608 4543 2f31 2e30 2e30 ca06  ..1...EC/1.0.0..
0000090: 1d6d 6f64 7365 6375 7269 7479 5f73 6865  .modsecurity_she
00000a0: 6c6c 7368 6f63 6b5f 312e 636f 6e66 e212  llshock_1.conf..
00000b0: 0d43 5645 2d32 3031 342d 3632 3731 a01f  .CVE-2014-6271..
00000c0: 0ba0 1f12 a01f 13c0 bb01 0080 fa01 00    ...............
```


