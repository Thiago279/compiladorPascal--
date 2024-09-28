#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Definições do analisador léxico
typedef enum {
    ERRO,
    IDENTIFICADOR,
    NUMERO,
    COMENTARIO,
    AND,
    BEGIN,
    BOOLEAN,
    ELIF,
    ELSE,
    END,
    FALSE,
    FOR,
    IF,
    INTEGER,
    NOT,
    OF,
    OR,
    PROGRAM,
    READ,
    SET,
    TO,
    TRUE,
    WRITE,
    PONTO_VIRGULA,
    VIRGULA,
    MAIS,          // Para '+'
    MENOS,         // Para '-'
    MAIOR,         // Para '>'
    MENOR,         // Para '<'
    IGUAL,        
    DOIS_PONTOS,
    ABRE_PAR,
    FECHA_PAR,
    EOS
} TAtomo;

typedef struct {
    TAtomo atomo;
    int linha;
    int atributo_numero;
    char atributo_ID[16];
    char comentario[256];
} TInfoAtomo;

// Variáveis globais
char *buffer;
int contaLinha = 1;
char *original_buffer;

char *msgAtomo[] = {
    "ERRO",
    "IDENTIFICADOR",
    "NUMERO",
    "COMENTARIO",
    "AND",
    "BEGIN",
    "BOOLEAN",
    "ELIF",
    "ELSE",
    "END",
    "FALSE",
    "FOR",
    "IF",
    "INTEGER",
    "NOT",
    "OF",
    "OR",
    "PROGRAM",
    "READ",
    "SET",
    "TO",
    "TRUE",
    "WRITE",
    "PONTO_VIRGULA",
    "VIRGULA",
    "MAIS",        
    "MENOS",         
    "MAIOR",        
    "MENOR",         
    "IGUAL",        
    "DOIS_PONTOS",
    "ABRE_PAR",
    "FECHA_PAR",
    "EOS"
};

const char* token_to_string(TAtomo token) {
    switch (token) {
        case PROGRAM: return "program";
        case BEGIN: return "begin";
        case END: return "end";
        case INTEGER: return "integer";
        case BOOLEAN: return "boolean";
        case IF: return "if";
        case ELIF: return "elif";
        case READ: return "read";
        case WRITE: return "write";
        case SET: return "set";
        case TO: return "to";
        case COMENTARIO: return "comentario";
        case ';': return "ponto_virgula";
        case ',': return "virgula";
        case '(': return "abre_par";
        case ')': return "fecha_par";

        default:
            if (token < 256) {
                static char str[2];
                str[0] = (char)token;
                str[1] = '\0';
                return str;
            }
            return msgAtomo[token];
    }
}

// Protótipos de funções do analisador léxico
char* ler_arquivo(const char* nome_arquivo);
TInfoAtomo obter_atomo();
TInfoAtomo reconhece_id();
TInfoAtomo reconhece_num();
TInfoAtomo reconhece_comentario();

// Implementações das funções do analisador léxico
char* ler_arquivo(const char* nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo %s\n", nome_arquivo);
        exit(1);
    }

    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    rewind(arquivo);

    char *conteudo = (char *)malloc(tamanho_arquivo + 1);
    if (!conteudo) {
        printf("Erro ao alocar memória\n");
        exit(1);
    }

    fread(conteudo, 1, tamanho_arquivo, arquivo);
    conteudo[tamanho_arquivo] = '\0';
    fclose(arquivo);
    
    original_buffer = conteudo;
    
    return conteudo;
}

TInfoAtomo obter_atomo() {
    TInfoAtomo info_atomo;

    while (1) {
        while (*buffer == ' ' || *buffer == '\n' || *buffer == '\t' || *buffer == '\r') {
            if (*buffer == '\n')
                contaLinha++;
            buffer++;
        }

        if (*buffer == '#' || (*buffer == '{' && *(buffer + 1) == '-')) {
            info_atomo = reconhece_comentario();
            printf("# %d: %s\n", info_atomo.linha, msgAtomo[info_atomo.atomo]);
            // Continue procurando o próximo átomo após o comentário
            continue;
        }

        if (islower(*buffer)) {
            info_atomo = reconhece_id();
        } else if (*buffer == '0' && *(buffer + 1) == 'b') {
            info_atomo = reconhece_num();
        } else if (*buffer == 0) {
            info_atomo.atomo = EOS;
        } else {
            info_atomo.atomo = *buffer;
            buffer++;
        }

        break;
    }

    info_atomo.linha = contaLinha;
    return info_atomo;
}

// Implementar reconhece_id(), reconhece_num() e reconhece_comentario() aqui
TInfoAtomo reconhece_comentario() {
    TInfoAtomo info_atomo;
    info_atomo.atomo = COMENTARIO;
    info_atomo.linha = contaLinha;

    if (*buffer == '#') {
        // Comentário de uma linha
        while (*buffer != '\n' && *buffer != 0) {
            buffer++;
        }
        if (*buffer == '\n') {
            contaLinha++;
            buffer++;
        }
    } else if (*buffer == '{' && *(buffer + 1) == '-') {
        // Comentário de várias linhas
        buffer += 2;
        while (!(*buffer == '-' && *(buffer + 1) == '}') && *buffer != 0) {
            if (*buffer == '\n')
                contaLinha++;
            buffer++;
        }
        if (*buffer == '-' && *(buffer + 1) == '}') {
            buffer += 2;
        }
    }

    return info_atomo;
}

TInfoAtomo reconhece_num() {
    TInfoAtomo info_atomo;
    info_atomo.atomo = ERRO;

    // Verifica se começa com '0b'
    if (*buffer == '0' && *(buffer + 1) == 'b') {
        buffer += 2; // Pula o '0b'

        // Verifica se a sequência é válida (0 ou 1)
        int valor = 0;
        if (*buffer != '0' && *buffer != '1') {
            return info_atomo; // Se não tiver ao menos um dígito válido, retorna erro
        }

        while (*buffer == '0' || *buffer == '1') {
            valor = (valor << 1) + (*buffer - '0'); // Converte de binário para decimal
            buffer++;
        }

        info_atomo.atributo_numero = valor;
        info_atomo.atomo = NUMERO;
    }

    return info_atomo;
}

// IDENTIFICADOR -> LETRA_MINUSCULA (LETRA_MINUSCULA | DIGITO | _ )*
TInfoAtomo reconhece_id() {
    TInfoAtomo info_atomo;
    info_atomo.atomo = ERRO;
    int contaChar = 1;
    char *iniID = buffer;

    // já temos uma letra minúscula
    buffer++;

q1:
    if (islower(*buffer) || isdigit(*buffer) || *buffer == '_') {
        buffer++;
        contaChar++;
        if (contaChar > 15) {
            info_atomo.atomo = ERRO;
            return info_atomo;
        }
        goto q1;
    }

    if (isupper(*buffer))
        return info_atomo;

    strncpy(info_atomo.atributo_ID, iniID, buffer - iniID);
    info_atomo.atributo_ID[buffer - iniID] = '\0';

    // Verifica se é palavra reservada
    if (strcmp(info_atomo.atributo_ID, "and") == 0) info_atomo.atomo = AND;
    else if (strcmp(info_atomo.atributo_ID, "begin") == 0) info_atomo.atomo = BEGIN;
    else if (strcmp(info_atomo.atributo_ID, "boolean") == 0) info_atomo.atomo = BOOLEAN;
    else if (strcmp(info_atomo.atributo_ID, "elif") == 0) info_atomo.atomo = ELIF;
    else if (strcmp(info_atomo.atributo_ID, "else") == 0) info_atomo.atomo = ELSE;
    else if (strcmp(info_atomo.atributo_ID, "end") == 0) info_atomo.atomo = END;
    else if (strcmp(info_atomo.atributo_ID, "false") == 0) info_atomo.atomo = FALSE;
    else if (strcmp(info_atomo.atributo_ID, "for") == 0) info_atomo.atomo = FOR;
    else if (strcmp(info_atomo.atributo_ID, "if") == 0) info_atomo.atomo = IF;
    else if (strcmp(info_atomo.atributo_ID, "integer") == 0) info_atomo.atomo = INTEGER;
    else if (strcmp(info_atomo.atributo_ID, "not") == 0) info_atomo.atomo = NOT;
    else if (strcmp(info_atomo.atributo_ID, "of") == 0) info_atomo.atomo = OF;
    else if (strcmp(info_atomo.atributo_ID, "or") == 0) info_atomo.atomo = OR;
    else if (strcmp(info_atomo.atributo_ID, "program") == 0) info_atomo.atomo = PROGRAM;
    else if (strcmp(info_atomo.atributo_ID, "read") == 0) info_atomo.atomo = READ;
    else if (strcmp(info_atomo.atributo_ID, "set") == 0) info_atomo.atomo = SET;
    else if (strcmp(info_atomo.atributo_ID, "to") == 0) info_atomo.atomo = TO;
    else if (strcmp(info_atomo.atributo_ID, "true") == 0) info_atomo.atomo = TRUE;
    else if (strcmp(info_atomo.atributo_ID, "write") == 0) info_atomo.atomo = WRITE;
    else info_atomo.atomo = IDENTIFICADOR;

    return info_atomo;
}

// Variável global para o lookahead do analisador sintático
TInfoAtomo lookahead;

// Protótipos de funções do analisador sintático
void consome();
void erro(TAtomo esperado, TAtomo encontrado);
int match(TAtomo esperado);
void expressao();
void comando();
void bloco();
void declaracao_de_variaveis();
void lista_variavel();
void tipo();
void comando_atribuicao();
void comando_condicional();
void comando_repeticao();
void comando_entrada();
void comando_saida();
void comando_composto();
void programa();

// Implementações das funções do analisador sintático
void consome() {
    lookahead = obter_atomo();
    
    switch (lookahead.atomo) {
        case IDENTIFICADOR:
            printf("# %d:identificador | %s\n", lookahead.linha, lookahead.atributo_ID);
            break;
        case NUMERO:
            printf("# %d:numero | %d\n", lookahead.linha, lookahead.atributo_numero);
            break;
        case COMENTARIO:
            printf("# %d:comentario\n", lookahead.linha);
            break;
        case EOS:
            printf("# %d:fim\n", lookahead.linha);
            break;
        default:
            printf("# %d:%s\n", lookahead.linha, token_to_string(lookahead.atomo));
    }
}


// Implementar as demais funções do analisador sintático aqui
// (Copie as implementações do seu arquivo sintatico.c)
// Funções para cada produção da gramática
void erro(TAtomo esperado, TAtomo encontrado) {
    printf("Erro sintático na linha %d: Esperado [%s], mas encontrado [%s].\n", 
           lookahead.linha, token_to_string(esperado), token_to_string(encontrado));
    
    if (esperado == IDENTIFICADOR) {
        printf("Esperava um identificador, mas encontrou '%s'.\n", lookahead.atributo_ID);
    } else if (esperado == NUMERO) {
        printf("Esperava um número, mas encontrou '%d'.\n", lookahead.atributo_numero);
    }
    
    exit(1);
}

int match(TAtomo esperado) {
    if (lookahead.atomo == esperado) {
        consome();
        return 1;
    } else {
        erro(esperado, lookahead.atomo);
        return 0;
    }
}


void expressao(); // Prototipando expressao para uso posterior

void comando();
void bloco();
void declaracao_de_variaveis();
void lista_variavel();
void tipo();
void comando_atribuicao();
void comando_condicional();
void comando_repeticao();
void comando_entrada();
void comando_saida();
void comando_composto();

// <programa> ::= program identificador “;” <bloco> “.”
void programa() {
    while (lookahead.atomo != PROGRAM && lookahead.atomo != EOS) {
        consome();
    }

    match(PROGRAM);
    match(IDENTIFICADOR);
    match(';');
    bloco();
    match('.');
}


// <bloco>::= <declaracao_de_variaveis> <comando_composto>
void bloco() {
    declaracao_de_variaveis();
    comando_composto();
}

// <declaracao_de_variaveis> ::= {<tipo> <lista_variavel> “;”}
void declaracao_de_variaveis() {
    while (lookahead.atomo == INTEGER || lookahead.atomo == BOOLEAN) {
        tipo();
        lista_variavel();
        match(';');
    }
}

// <tipo> ::= integer | boolean
void tipo() {
    if (lookahead.atomo == INTEGER) {
        match(INTEGER);
    } else if (lookahead.atomo == BOOLEAN) {
        match(BOOLEAN);
    } else {
        erro(INTEGER, lookahead.atomo); // Espera um tipo
    }
}

// <lista_variavel> ::= identificador { “,” identificador }
void lista_variavel() {
    match(IDENTIFICADOR);
    while (lookahead.atomo == ',') {
        match(',');
        match(IDENTIFICADOR);
    }
}

// <comando_composto> ::= begin <comando> {“;”<comando>} end
void comando_composto() {
    match(BEGIN);
    comando();
    while (lookahead.atomo == ';') {
        match(';');
        comando();
    }
    match(END);
}

// <comando> ::= <comando_atribuicao> |
//               <comando_condicional> |
//               <comando_repeticao> |
//               <comando_entrada> |
//               <comando_saida> |
//               <comando_composto>
void comando() {
    switch (lookahead.atomo) {
        case SET:
            comando_atribuicao();
            break;
        case IF:
            comando_condicional();
            break;
        case FOR:
            comando_repeticao();
            break;
        case READ:
            comando_entrada();
            break;
        case WRITE:
            comando_saida();
            break;
        case BEGIN:
            comando_composto();
            break;
        default:
            erro(SET, lookahead.atomo); // Espera um comando;
    }
}

// <comando_atribuicao> ::= set identificador to <expressao>
void comando_atribuicao() {
    match(SET);
    match(IDENTIFICADOR);
    match(TO);
    expressao();
}

// <comando_condicional> ::= if <expressao> “:” <comando> [elif <comando>]
void comando_condicional() {
    match(IF);
    expressao();
    match(':');
    comando();
    if (lookahead.atomo == ELIF) {
        match(ELIF);
        comando();
    }
}

// <comando_repeticao> ::= for identificador of <expressão> to <expressão> “:” <comando>
void comando_repeticao() {
    match(FOR);
    match(IDENTIFICADOR);
    match(OF);
    expressao();
    match(TO);
    expressao();
    match(':');
    comando();
}

// <comando_entrada> ::= read “(“ <lista_variavel> “)”
void comando_entrada() {
    match(READ);
    match('(');
    lista_variavel();
    match(')');
}

// <comando_saida> ::= write “(“ <expressao> { “,” <expressao> } “)”
void comando_saida() {
    match(WRITE);
    match('(');
    expressao();
    while (lookahead.atomo == ',') {
        match(',');
        expressao();
    }
    match(')');
}

// Implementar a análise de expressões simples
void expressao() {
    // Implementação básica: aceita números ou identificadores por enquanto
    if (lookahead.atomo == NUMERO) {
        match(NUMERO);
    } else if (lookahead.atomo == IDENTIFICADOR) {
        match(IDENTIFICADOR);
    } else {
        erro(NUMERO, lookahead.atomo); // Espera um número ou identificador
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <arquivo_fonte>\n", argv[0]);
        return 1;
    }

    buffer = ler_arquivo(argv[1]);

    consome();
    programa();

    printf("%d linhas analisadas, programa sintaticamente correto\n", contaLinha);

    free(original_buffer);
    return 0;
}
