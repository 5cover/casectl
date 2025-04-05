// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vsc from 'vscode';

//const output = vsc.window.createOutputChannel('casectl'); // unused

export function activate(ctx: vsc.ExtensionContext) {
    const decorationType = vsc.window.createTextEditorDecorationType({
        fontStyle: 'italic',
    });

    const diagnostics = vsc.languages.createDiagnosticCollection('casectl');
    ctx.subscriptions.push(diagnostics);


    const updateDecorations = () => {
        const editor = vsc.window.activeTextEditor;
        if (!editor) { return; }
        const [decorations, diags] = getDecorations(editor);
        editor.setDecorations(decorationType, decorations);
        diagnostics.set(editor.document.uri, diags);
    };

    ctx.subscriptions.push(
        vsc.workspace.onDidChangeTextDocument(e => { if (e.document.isDirty) { updateDecorations(); } }),
        vsc.workspace.onDidOpenTextDocument(updateDecorations)
    );

    updateDecorations(); // Run once on activation
}

function getDecorations(editor: vsc.TextEditor): [vsc.DecorationOptions[], vsc.Diagnostic[]] {
    const decorations: vsc.DecorationOptions[] = [];
    const diagnostics: vsc.Diagnostic[] = [];
    const text = editor.document.getText();

    if (/\p{Ll}/u.test(text)) {
        return [decorations, diagnostics];
    }

    const EN = '¤';
    let iSpanStart = -1;

    for (let i = 0; i < text.length; ++i) {
        if (text.charAt(i) === EN) {
            if (text.charAt(i + 1) === EN) {
                ++i;
                continue;
            }
            if (iSpanStart === -1) { // opening a span
                iSpanStart = i;
            } else { // closing a span
                decorations.push({
                    range: new vsc.Range(
                        editor.document.positionAt(iSpanStart),
                        editor.document.positionAt(i),
                    )
                });
                iSpanStart = -1;
            }
        }
    }

    if (iSpanStart !== -1) {
        const pos = editor.document.positionAt(iSpanStart);
        const range = new vsc.Range(pos, pos.translate(0, 1)); // one character
        diagnostics.push({
            range: range,
            message: 'Unterminated literal span starting here. Did you forget to escape it?',
            severity: vsc.DiagnosticSeverity.Warning,
            source: 'casectl',
        });
    }

    return [decorations, diagnostics];
}

/*// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

    // Use the console to output diagnostic information (console.log) and errors (console.error)
    // This line of code will only be executed once when your extension is activated
    console.log('Congratulations, your extension "casectl" is now active!');

    // The command has been defined in the package.json file
    // Now provide the implementation of the command with registerCommand
    // The commandId parameter must match the command field in package.json
    const disposable = vscode.commands.registerCommand('casectl.helloWorld', () => {
        // The code you place here will be executed every time your command is executed
        // Display a message box to the user
        vscode.window.showInformationMessage('Hello World from casectl!');
    });

    context.subscriptions.push(disposable);
}*/

// This method is called when your extension is deactivated
export function deactivate() { }
