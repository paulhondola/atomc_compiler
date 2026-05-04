project('AtomC-Compiler', 'c', version : '1.0.0', default_options : ['warning_level=3', 'optimization=3'])

compiler_args = [
    '-std=c23',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-g'
]

src_files = files(
    'src/utils/utils.c',
    'src/frontend/lexer.c',
    'src/frontend/parser.c',
    'src/frontend/token.c',
    'src/frontend/stream.c',
    'src/analyzer/symbol.c',
    'src/analyzer/domain_analyzer.c',
    'src/analyzer/type.c',
    'src/vm/instruction.c',
    'src/vm/vm.c',
)

inc_dir = include_directories('include')

exe = executable('atomcc', 'src/main.c', src_files, include_directories : inc_dir, c_args : compiler_args)

# ------- LEXER -------
lexer_files = files()
test_lexer = executable('test_lexer',
                        'tests/src/test_lexer.c',
                        src_files,
                        include_directories : inc_dir,
                        c_args : compiler_args
)

test('test_lexer', test_lexer, args : lexer_files, depends : [exe])

# ------- PARSER -------
parser_files = files()
test_parser = executable('test_parser',
                         'tests/src/test_parser.c',
                         src_files,
                         include_directories : inc_dir,
                         c_args : compiler_args
)

test('test_parser', test_parser, args : parser_files, depends : [exe])

# ------- DOMAIN ANALYZER -------
domain_analyzer_files = files()
test_domain_analyzer = executable('test_domain_analyzer',
                                  'tests/src/test_domain_analyzer.c',
                                  src_files,
                                  include_directories : inc_dir,
                                  c_args : compiler_args
)

test('test_domain_analyzer', test_domain_analyzer, args : domain_analyzer_files, depends : [exe])

# ------- TYPE ANALYZER -------
type_analyzer_files = files()
test_type_analyzer = executable('test_type_analyzer',
                                'tests/src/test_type_analyzer.c',
                                src_files,
                                include_directories : inc_dir,
                                c_args : compiler_args
)

test('test_type_analyzer', test_type_analyzer, args : type_analyzer_files, depends : [exe])

# ------- VIRTUAL MACHINE -------
vm_files = files()
test_vm = executable('test_vm',
                        'tests/src/test_vm.c',
                        src_files,
                        include_directories : inc_dir,
                        c_args : compiler_args
)

test('test_vm', test_vm, args : vm_files, depends : [exe])

# ------- CODE GENERATOR -------
code_generator_files = files()
test_code_generator = executable('test_code_generator',
                                  'tests/src/test_code_generator.c',
                                  src_files,
                                  include_directories : inc_dir,
                                  c_args : compiler_args
)

test('test_code_generator', test_code_generator, args : code_generator_files, depends : [exe])
