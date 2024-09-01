lib:
  g++ -std=c++11 -I../tree-sitter/lib/include -dynamiclib card.cpp -I ../ts_parsers/tree-sitter-sql/bindings/swift/TreeSitterSQL ../ts_parsers/sql_syn.o ../tree-sitter/libtree-sitter.a -o libcard.dylib

main:
  g++ -std=c++11 -I../tree-sitter/lib/include main.cpp -I ../ts_parsers/tree-sitter-sql/bindings/swift/TreeSitterSQL ../ts_parsers/sql_syn.o ../tree-sitter/libtree-sitter.a -o card
