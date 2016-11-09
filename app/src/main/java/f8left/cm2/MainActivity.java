package f8left.cm2;

import android.content.SharedPreferences;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    EditText mEdit;
    Button mButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mEdit = (EditText)findViewById(R.id.mKeyEdit);
        mButton = (Button)findViewById(R.id.mCheckBtn);

        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String flag = mEdit.getText().toString();
                if(check(flag)) {
                    Toast.makeText(MainActivity.this, "great:flag{" + flag + "}",
                            Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(MainActivity.this, "Error, try again",
                            Toast.LENGTH_SHORT).show();
                }
            }
        });
        mEdit.setText("ThatIsEnd,Thanks");
    }

    private native boolean check(String flag);
}
